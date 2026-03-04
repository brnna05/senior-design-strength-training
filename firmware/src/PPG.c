/*
 * PPG.c - MAX30102 PPG Sensor Driver + Application
 * Senior Design - Strength Training
 *
 * Contains:
 * - Zephyr sensor driver (device registration, init, fetch, channel get)
 * - Heart rate calculation via peak detection
 * - Application API: ppg_init() and ppg_read() for use in main.c
 */

#define DT_DRV_COMPAT maxim_max30102

#include "PPG.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(MAX30102, CONFIG_SENSOR_LOG_LEVEL);

/* ══════════════════════════════════════════════════════════════════════════
 * DRIVER LAYER
 * Handles I2C communication, device registration, and heart rate algorithm.
 * ══════════════════════════════════════════════════════════════════════════ */

/* ── Heart Rate: Improved Peak Detection ────────────────────────────────────
 * Improvements over naive version:
 * 1. Low pass filter to smooth noise before peak detection
 * 2. Minimum peak distance to avoid counting noise spikes as beats
 *    (min distance = 200Hz / 180BPM_max = ~67 samples)
 * 3. Valid BPM range check (40-180 BPM) to reject garbage readings
 * 4. BPM averaging over last 4 readings for stability
 * ───────────────────────────────────────────────────────────────────────── */

#define HR_MIN_BPM          40
#define HR_MAX_BPM          180
#define HR_MIN_PEAK_DIST    (HR_SAMPLE_RATE * 60 / HR_MAX_BPM)  /* ~67 samples */
#define HR_SMOOTH_SIZE      5    /* low pass filter window size */
#define HR_AVG_SIZE         4    /* number of BPM readings to average */

static int32_t bpm_history[HR_AVG_SIZE] = {0};
static uint8_t bpm_history_idx = 0;
static uint8_t bpm_history_count = 0;
struct sensor_value ir_val;

/* Devices */
const struct device *dev = DEVICE_DT_GET_ANY(maxim_max30102);

static int32_t max30102_calc_heart_rate(struct max30102_data *data)
{
    if (!data->ir_buf_full) {
        return -1;
    }

    /* Step 1: calculate mean to check finger presence */
    uint64_t sum = 0;
    for (int i = 0; i < HR_BUFFER_SIZE; i++) {
        sum += data->ir_buffer[i];
    }
    uint32_t mean = (uint32_t)(sum / HR_BUFFER_SIZE);

    if (mean < HR_MIN_VALID_IR) {
        printk("MAX30102: No finger detected (IR mean=%u)\n", mean);
        bpm_history_count = 0; /* reset averaging when finger removed */
        return -1;
    }

    /* Step 2: low pass filter - smooth signal with moving average */
    uint32_t smoothed[HR_BUFFER_SIZE];
    for (int i = 0; i < HR_BUFFER_SIZE; i++) {
        uint64_t window_sum = 0;
        int count = 0;
        for (int j = i - HR_SMOOTH_SIZE / 2;
             j <= i + HR_SMOOTH_SIZE / 2; j++) {
            if (j >= 0 && j < HR_BUFFER_SIZE) {
                window_sum += data->ir_buffer[j];
                count++;
            }
        }
        smoothed[i] = (uint32_t)(window_sum / count);
    }

    /* Step 3: recalculate mean of smoothed signal as threshold */
    uint64_t smooth_sum = 0;
    for (int i = 0; i < HR_BUFFER_SIZE; i++) {
        smooth_sum += smoothed[i];
    }
    uint32_t smooth_mean = (uint32_t)(smooth_sum / HR_BUFFER_SIZE);

    /* Step 4: count peaks with minimum distance enforcement */
    int peaks = 0;
    int last_peak = -HR_MIN_PEAK_DIST; /* allow first peak immediately */
    bool above = (smoothed[0] > smooth_mean);

    for (int i = 1; i < HR_BUFFER_SIZE; i++) {
        bool now_above = (smoothed[i] > smooth_mean);
        if (!above && now_above) {
            /* upward crossing found - check min distance from last peak */
            if ((i - last_peak) >= HR_MIN_PEAK_DIST) {
                peaks++;
                last_peak = i;
            }
        }
        above = now_above;
    }

    /* Step 5: calculate raw BPM */
    int32_t raw_bpm = (int32_t)(peaks * 60 * HR_SAMPLE_RATE / HR_BUFFER_SIZE);

    /* Step 6: validate BPM range */
    if (raw_bpm < HR_MIN_BPM || raw_bpm > HR_MAX_BPM) {
        return -1;
    }

    /* Step 7: average over last HR_AVG_SIZE readings for stability */
    bpm_history[bpm_history_idx] = raw_bpm;
    bpm_history_idx = (bpm_history_idx + 1) % HR_AVG_SIZE;
    if (bpm_history_count < HR_AVG_SIZE) {
        bpm_history_count++;
    }

    int64_t bpm_sum = 0;
    for (int i = 0; i < bpm_history_count; i++) {
        bpm_sum += bpm_history[i];
    }

    return (int32_t)(bpm_sum / bpm_history_count);
}

/* ── Sample Fetch ───────────────────────────────────────────────────────── */
static int max30102_sample_fetch(const struct device *dev,
                                 enum sensor_channel chan)
{
    struct max30102_data *data = dev->data;
    const struct max30102_config *config = dev->config;

    /* Read 6 bytes from FIFO: 3 bytes RED + 3 bytes IR */
    uint8_t buffer[MAX30102_MAX_NUM_CHANNELS * MAX30102_BYTES_PER_CHANNEL];

    if (i2c_burst_read_dt(&config->i2c, MAX30102_REG_FIFO_DATA,
                          buffer, sizeof(buffer))) {
        LOG_ERR("Failed to read FIFO");
        return -EIO;
    }

    /* Parse RED: bytes 0,1,2 → 18-bit value */
    data->raw[MAX30102_LED_CHANNEL_RED] =
        ((uint32_t)buffer[0] << 16) |
        ((uint32_t)buffer[1] <<  8) |
         (uint32_t)buffer[2];
    data->raw[MAX30102_LED_CHANNEL_RED] &= MAX30102_FIFO_DATA_MASK;

    /* Parse IR: bytes 3,4,5 → 18-bit value */
    data->raw[MAX30102_LED_CHANNEL_IR] =
        ((uint32_t)buffer[3] << 16) |
        ((uint32_t)buffer[4] <<  8) |
         (uint32_t)buffer[5];
    data->raw[MAX30102_LED_CHANNEL_IR] &= MAX30102_FIFO_DATA_MASK;

    /* Clear interrupt status register */
    uint8_t status;
    i2c_reg_read_byte_dt(&config->i2c, MAX30102_REG_INT_STS1, &status);

    /* Update rolling IR buffer for heart rate */
    data->ir_buffer[data->ir_buf_idx] = data->raw[MAX30102_LED_CHANNEL_IR];
    data->ir_buf_idx++;

    if (data->ir_buf_idx >= HR_BUFFER_SIZE) {
        data->ir_buf_idx = 0;
        data->ir_buf_full = true;

        /* Recalculate BPM every 1 second */
        int32_t bpm = max30102_calc_heart_rate(data);
        if (bpm > 0) {
            data->bpm = bpm;
            printk("MAX30102: Heart Rate = %d BPM\n", bpm);
        }
    }

    return 0;
}

/* ── Channel Get ────────────────────────────────────────────────────────── */
static int max30102_channel_get(const struct device *dev,
                                enum sensor_channel chan,
                                struct sensor_value *val)
{
    struct max30102_data *data = dev->data;

    switch (chan) {
    case SENSOR_CHAN_RED:
        val->val1 = data->raw[MAX30102_LED_CHANNEL_RED];
        val->val2 = 0;
        break;
    case SENSOR_CHAN_IR:
        val->val1 = data->raw[MAX30102_LED_CHANNEL_IR];
        val->val2 = 0;
        break;
    default:
        LOG_ERR("Unsupported channel");
        return -ENOTSUP;
    }

    return 0;
}

/* ── Driver API ─────────────────────────────────────────────────────────── */
static const struct sensor_driver_api max30102_driver_api = {
    .sample_fetch = max30102_sample_fetch,
    .channel_get  = max30102_channel_get,
};

/* ── Device Init ────────────────────────────────────────────────────────── */
static int max30102_init(const struct device *dev)
{
    const struct max30102_config *config = dev->config;
    struct max30102_data *data = dev->data;
    uint8_t part_id;
    uint8_t mode_cfg;

    memset(data, 0, sizeof(*data));

    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    /* Verify chip is MAX30102 */
    if (i2c_reg_read_byte_dt(&config->i2c, MAX30102_REG_PART_ID, &part_id)) {
        LOG_ERR("Could not read Part ID");
        return -EIO;
    }
    if (part_id != MAX30102_PART_ID) {
        LOG_ERR("Wrong Part ID: got 0x%02x expected 0x%02x",
                part_id, MAX30102_PART_ID);
        return -EIO;
    }
    LOG_INF("MAX30102 found, Part ID: 0x%02x", part_id);

    /* Reset chip */
    if (i2c_reg_write_byte_dt(&config->i2c, MAX30102_REG_MODE_CFG,
                               MAX30102_MODE_CFG_RESET_MASK)) {
        LOG_ERR("Reset failed");
        return -EIO;
    }

    /* Wait for reset to clear */
    do {
        if (i2c_reg_read_byte_dt(&config->i2c,
                                  MAX30102_REG_MODE_CFG, &mode_cfg)) {
            LOG_ERR("Could not read MODE_CFG after reset");
            return -EIO;
        }
    } while (mode_cfg & MAX30102_MODE_CFG_RESET_MASK);

    /* Configure FIFO */
    if (i2c_reg_write_byte_dt(&config->i2c,
                               MAX30102_REG_FIFO_CFG, config->fifo)) {
        return -EIO;
    }

    /* Set mode (3 = SpO2: RED + IR) */
    if (i2c_reg_write_byte_dt(&config->i2c,
                               MAX30102_REG_MODE_CFG, config->mode)) {
        return -EIO;
    }

    /* Set SpO2 config (ADC range + sample rate + pulse width) */
    if (i2c_reg_write_byte_dt(&config->i2c,
                               MAX30102_REG_SPO2_CFG, config->spo2)) {
        return -EIO;
    }

    /* Set LED brightness */
    if (i2c_reg_write_byte_dt(&config->i2c,
                               MAX30102_REG_LED1_PA, config->led_pa[0])) {
        return -EIO;
    }
    if (i2c_reg_write_byte_dt(&config->i2c,
                               MAX30102_REG_LED2_PA, config->led_pa[1])) {
        return -EIO;
    }

    LOG_INF("MAX30102 initialized");
    printk("MAX30102: Ready. Place finger on sensor...\n");
    return 0;
}

/* ── Devicetree Instantiation ───────────────────────────────────────────── */
#define MAX30102_DEFINE(inst)                                                   \
    static struct max30102_data max30102_data_##inst;                           \
                                                                                \
    static const struct max30102_config max30102_config_##inst = {              \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                     \
                                                                                \
        .fifo = (DT_INST_PROP(inst, smp_ave)                                   \
                    << MAX30102_FIFO_CFG_SMP_AVE_SHIFT) |                       \
                COND_CODE_1(DT_INST_PROP(inst, fifo_rollover_en),               \
                    (MAX30102_FIFO_CFG_ROLLOVER_MASK |), ())                    \
                (DT_INST_PROP(inst, fifo_a_full)                               \
                    << MAX30102_FIFO_CFG_FULL_SHIFT),                           \
                                                                                \
        .mode = DT_INST_PROP(inst, mode),                                      \
                                                                                \
        .spo2 = (DT_INST_PROP(inst, adc_rge)                                   \
                    << MAX30102_SPO2_ADC_RGE_SHIFT) |                           \
                (DT_INST_PROP(inst, sr)                                        \
                    << MAX30102_SPO2_SR_SHIFT)       |                          \
                (MAX30102_PW_18BITS                                             \
                    << MAX30102_SPO2_PW_SHIFT),                                 \
                                                                                \
        .led_pa[0] = DT_INST_PROP(inst, led1_pa),                              \
        .led_pa[1] = DT_INST_PROP(inst, led2_pa),                              \
    };                                                                          \
                                                                                \
    DEVICE_DT_INST_DEFINE(inst,                                                 \
                          max30102_init,                                        \
                          NULL,                                                 \
                          &max30102_data_##inst,                                \
                          &max30102_config_##inst,                              \
                          POST_KERNEL,                                          \
                          CONFIG_SENSOR_INIT_PRIORITY,                          \
                          &max30102_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX30102_DEFINE)

static struct k_work ppg_work;
static struct gpio_callback gpio_cb;

K_TIMER_DEFINE(ppg_timer, PPG_handler, NULL);

/* Timer Callback (interrupt context) */
void PPG_handler(struct k_timer *timer_id)
{
    k_work_submit(&ppg_work);
}

/* Work Handler (work queue thread) */
static void ppg_work_handler(struct k_work *work)
{
    ppg_read();
}


int ppg_init(int sample_rate_hz)
{
    if (!device_is_ready(dev)) {
        printk("MAX30102 not ready\n");
        return -1;
    }
    ir_val.val1 = 0;
    ir_val.val2 = 0;

    /* Start periodic sampling timer */
    uint32_t ppg_period_us = 1000000U / (uint32_t)sample_rate_hz;
    k_work_init(&ppg_work, ppg_work_handler);
    k_timer_start(&ppg_timer, K_USEC(ppg_period_us), K_USEC(ppg_period_us));

    printk("PPG: initialized at %d Hz\n", sample_rate_hz);

    printk("MAX30102 ready\n");
    return 0;
}

void ppg_read(void)
{
    int ret = sensor_sample_fetch(dev);
    if (ret < 0) {
        printk("MAX30102 fetch failed: %d\n", ret);
        return;
    }

    sensor_channel_get(dev, SENSOR_CHAN_IR, &ir_val);
    //printk("IR: %d\n", ir_val.val1);
}

struct sensor_value *ppg_get_data(void) { return &ir_val; }