/*
 * PPG.c - MAX30102 PPG Sensor Driver + Application
 * Senior Design - Strength Training
 *
 * Heart rate algorithm: inter-peak interval timing.
 *
 * Old approach (broken):
 *   BPM = peak_count * 60 * SR / BUFFER_SIZE
 *   → only produces multiples of 60 (1 peak=60, 2=120, …)
 *
 * New approach:
 *   For every pair of adjacent peaks at positions p[i] and p[i+1]:
 *     interval_bpm = 60 * SR / (p[i+1] - p[i])
 *   BPM = average of all interval_bpm values in the window.
 *   With SR=200 and a 600-sample (3 s) window this resolves to ~1 BPM
 *   steps across the 40–180 BPM range.
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

/* ── Heart Rate Constants ───────────────────────────────────────────────── */
#define HR_MIN_BPM       40
#define HR_MAX_BPM       180

/* Minimum samples between two valid peaks at the fastest allowable rate */
#define HR_MIN_PEAK_DIST (HR_SAMPLE_RATE * 60 / HR_MAX_BPM)  /* ~67 samples */

/* Moving-average smoothing window (low-pass filter before peak detection) */
#define HR_SMOOTH_SIZE   5

/* Number of consecutive valid BPM readings to average for final output */
#define HR_AVG_SIZE      4

/* Maximum peaks we expect in a 3-second window (180 BPM → ~9 peaks) */
#define HR_MAX_PEAKS     20

static int32_t bpm_history[HR_AVG_SIZE] = {0};
static uint8_t bpm_history_idx   = 0;
static uint8_t bpm_history_count = 0;

/* Latest valid BPM written by ppg_read(), read by ppg_get_bpm() */
static int32_t latest_bpm = -1;

struct sensor_value ir_val;
const struct device *dev = DEVICE_DT_GET_ANY(maxim_max30102);

/* ── Heart Rate: Inter-Peak Interval Algorithm ──────────────────────────── */
static int32_t max30102_calc_heart_rate(struct max30102_data *data)
{
    if (!data->ir_buf_full) {
        return -1;
    }

    /* ── Step 1: finger-presence check via mean IR level ── */
    uint64_t sum = 0;
    for (int i = 0; i < HR_BUFFER_SIZE; i++) {
        sum += data->ir_buffer[i];
    }
    uint32_t mean = (uint32_t)(sum / HR_BUFFER_SIZE);

    if (mean < HR_MIN_VALID_IR) {
        printk("PPG: no finger (mean IR=%u)\n", mean);
        return -1;
    }

    /* ── Step 2: low-pass filter — moving average ── */
    uint32_t smoothed[HR_BUFFER_SIZE];
    for (int i = 0; i < HR_BUFFER_SIZE; i++) {
        uint64_t w = 0;
        int n = 0;
        for (int j = i - HR_SMOOTH_SIZE / 2;
             j <= i + HR_SMOOTH_SIZE / 2; j++) {
            if (j >= 0 && j < HR_BUFFER_SIZE) {
                w += data->ir_buffer[j];
                n++;
            }
        }
        smoothed[i] = (uint32_t)(w / n);
    }

    /* ── Step 3: adaptive threshold = mean of smoothed signal ── */
    uint64_t smooth_sum = 0;
    for (int i = 0; i < HR_BUFFER_SIZE; i++) {
        smooth_sum += smoothed[i];
    }
    uint32_t threshold = (uint32_t)(smooth_sum / HR_BUFFER_SIZE);

    /* ── Step 4: locate upward threshold-crossings (peak positions) ── */
    int peak_pos[HR_MAX_PEAKS];
    int peak_count = 0;
    int last_peak  = -HR_MIN_PEAK_DIST;
    bool above = (smoothed[0] > threshold);

    for (int i = 1; i < HR_BUFFER_SIZE && peak_count < HR_MAX_PEAKS; i++) {
        bool now_above = (smoothed[i] > threshold);
        if (!above && now_above) {
            if ((i - last_peak) >= HR_MIN_PEAK_DIST) {
                peak_pos[peak_count++] = i;
                last_peak = i;
            }
        }
        above = now_above;
    }

    /* Need at least 2 peaks to form 1 interval */
    if (peak_count < 2) {
        printk("PPG: only %d peak(s) found — need >= 2\n", peak_count);
        return -1;
    }

    /* ── Step 5: BPM from each inter-peak interval, then average ──
     *
     * BPM_i = 60 * HR_SAMPLE_RATE / (peak_pos[i+1] - peak_pos[i])
     *
     * Example: peaks at samples 80 and 247 → interval = 167 samples
     *   BPM = 60 * 200 / 167 = 71.9 → 72 BPM
     *
     * This gives ~1 BPM resolution vs the old "multiples of 60" behaviour.
     */
    int64_t bpm_acc = 0;
    int intervals   = peak_count - 1;

    for (int i = 0; i < intervals; i++) {
        int interval = peak_pos[i + 1] - peak_pos[i];
        bpm_acc += (int64_t)(60 * HR_SAMPLE_RATE) / interval;
    }

    int32_t raw_bpm = (int32_t)(bpm_acc / intervals);

    /* ── Step 6: range validation ── */
    if (raw_bpm < HR_MIN_BPM || raw_bpm > HR_MAX_BPM) {
        printk("PPG: raw BPM %d out of range\n", raw_bpm);
        return -1;
    }

    /* ── Step 7: rolling average over last HR_AVG_SIZE readings ── */
    bpm_history[bpm_history_idx] = raw_bpm;
    bpm_history_idx = (bpm_history_idx + 1) % HR_AVG_SIZE;
    if (bpm_history_count < HR_AVG_SIZE) {
        bpm_history_count++;
    }

    int64_t avg_acc = 0;
    for (int i = 0; i < bpm_history_count; i++) {
        avg_acc += bpm_history[i];
    }

    return (int32_t)(avg_acc / bpm_history_count);
}

/* ── Sample Fetch ───────────────────────────────────────────────────────── */
static int max30102_sample_fetch(const struct device *dev,
                                 enum sensor_channel chan)
{
    struct max30102_data *data   = dev->data;
    const struct max30102_config *config = dev->config;

    uint8_t buffer[MAX30102_MAX_NUM_CHANNELS * MAX30102_BYTES_PER_CHANNEL];

    if (i2c_burst_read_dt(&config->i2c, MAX30102_REG_FIFO_DATA,
                          buffer, sizeof(buffer))) {
        LOG_ERR("Failed to read FIFO");
        return -EIO;
    }

    data->raw[MAX30102_LED_CHANNEL_RED] =
        ((uint32_t)buffer[0] << 16) |
        ((uint32_t)buffer[1] <<  8) |
         (uint32_t)buffer[2];
    data->raw[MAX30102_LED_CHANNEL_RED] &= MAX30102_FIFO_DATA_MASK;

    data->raw[MAX30102_LED_CHANNEL_IR] =
        ((uint32_t)buffer[3] << 16) |
        ((uint32_t)buffer[4] <<  8) |
         (uint32_t)buffer[5];
    data->raw[MAX30102_LED_CHANNEL_IR] &= MAX30102_FIFO_DATA_MASK;

    uint8_t status;
    i2c_reg_read_byte_dt(&config->i2c, MAX30102_REG_INT_STS1, &status);

    data->ir_buffer[data->ir_buf_idx] = data->raw[MAX30102_LED_CHANNEL_IR];
    data->ir_buf_idx++;

    if (data->ir_buf_idx >= HR_BUFFER_SIZE) {
        data->ir_buf_idx = 0;
        data->ir_buf_full = true;
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
    uint8_t part_id, mode_cfg;

    memset(data, 0, sizeof(*data));

    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    if (i2c_reg_read_byte_dt(&config->i2c, MAX30102_REG_PART_ID, &part_id)) {
        LOG_ERR("Could not read Part ID");
        return -EIO;
    }
    if (part_id != MAX30102_PART_ID) {
        LOG_ERR("Wrong Part ID: 0x%02x (expected 0x%02x)", part_id, MAX30102_PART_ID);
        return -EIO;
    }

    if (i2c_reg_write_byte_dt(&config->i2c, MAX30102_REG_MODE_CFG,
                               MAX30102_MODE_CFG_RESET_MASK)) {
        return -EIO;
    }
    do {
        if (i2c_reg_read_byte_dt(&config->i2c, MAX30102_REG_MODE_CFG, &mode_cfg)) {
            return -EIO;
        }
    } while (mode_cfg & MAX30102_MODE_CFG_RESET_MASK);

    if (i2c_reg_write_byte_dt(&config->i2c, MAX30102_REG_FIFO_CFG, config->fifo) ||
        i2c_reg_write_byte_dt(&config->i2c, MAX30102_REG_MODE_CFG, config->mode) ||
        i2c_reg_write_byte_dt(&config->i2c, MAX30102_REG_SPO2_CFG, config->spo2) ||
        i2c_reg_write_byte_dt(&config->i2c, MAX30102_REG_LED1_PA,  config->led_pa[0]) ||
        i2c_reg_write_byte_dt(&config->i2c, MAX30102_REG_LED2_PA,  config->led_pa[1])) {
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
        .fifo = (DT_INST_PROP(inst, smp_ave)                                   \
                    << MAX30102_FIFO_CFG_SMP_AVE_SHIFT) |                       \
                COND_CODE_1(DT_INST_PROP(inst, fifo_rollover_en),               \
                    (MAX30102_FIFO_CFG_ROLLOVER_MASK |), ())                    \
                (DT_INST_PROP(inst, fifo_a_full)                               \
                    << MAX30102_FIFO_CFG_FULL_SHIFT),                           \
        .mode = DT_INST_PROP(inst, mode),                                      \
        .spo2 = (DT_INST_PROP(inst, adc_rge)                                   \
                    << MAX30102_SPO2_ADC_RGE_SHIFT) |                           \
                (DT_INST_PROP(inst, sr)  << MAX30102_SPO2_SR_SHIFT) |           \
                (MAX30102_PW_18BITS      << MAX30102_SPO2_PW_SHIFT),            \
        .led_pa[0] = DT_INST_PROP(inst, led1_pa),                              \
        .led_pa[1] = DT_INST_PROP(inst, led2_pa),                              \
    };                                                                          \
                                                                                \
    DEVICE_DT_INST_DEFINE(inst, max30102_init, NULL,                            \
                          &max30102_data_##inst, &max30102_config_##inst,       \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,             \
                          &max30102_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX30102_DEFINE)

/* ══════════════════════════════════════════════════════════════════════════
 * APPLICATION LAYER
 * ══════════════════════════════════════════════════════════════════════════ */

static struct k_work ppg_work;

static void ppg_work_handler(struct k_work *work)
{
    ppg_read();
}

void PPG_handler(struct k_timer *timer_id)
{
    k_work_submit(&ppg_work);
}

K_TIMER_DEFINE(ppg_timer, PPG_handler, NULL);

int ppg_init(int sample_rate_hz)
{
    if (!device_is_ready(dev)) {
        printk("MAX30102 not ready\n");
        return -1;
    }
    ir_val.val1  = 0;
    ir_val.val2  = 0;
    latest_bpm   = -1;

    uint32_t period_us = 1000000U / (uint32_t)sample_rate_hz;
    k_work_init(&ppg_work, ppg_work_handler);
    k_timer_start(&ppg_timer, K_USEC(period_us), K_USEC(period_us));

    printk("PPG: initialized at %d Hz, buffer=%d samples (%.1f s)\n",
           sample_rate_hz, HR_BUFFER_SIZE,
           (float)HR_BUFFER_SIZE / sample_rate_hz);
    return 0;
}

void ppg_read(void)
{
    if (sensor_sample_fetch(dev) < 0) {
        printk("MAX30102 fetch failed\n");
        return;
    }

    sensor_channel_get(dev, SENSOR_CHAN_IR, &ir_val);

    /*
     * Recalculate once per full buffer revolution (every HR_BUFFER_SIZE
     * samples). ir_buf_idx just wrapped to 0 at that moment.
     */
    struct max30102_data *data = dev->data;
    if (data->ir_buf_full && data->ir_buf_idx == 0) {
        int32_t bpm = max30102_calc_heart_rate(data);
        if (bpm > 0) {
            latest_bpm = bpm;
            printk("PPG: BPM = %d\n", (int)bpm);
        }
    }
}

struct sensor_value *ppg_get_data(void) { return &ir_val; }
int32_t ppg_get_bpm(void) { return latest_bpm; }