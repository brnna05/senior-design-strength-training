/*
 * PPG.c - MAX30102 PPG Sensor Driver + Application
 * Senior Design - Strength Training
 *
 * Heart rate algorithm: Inter-Beat Interval (IBI) tracking.
 *
 * Translated from the open-source PulseSensor Arduino ISR into Zephyr's
 * timer + work-queue model.
 *
 * Original Arduino approach (2 ms ISR at 500 Hz, 10-bit ADC):
 *   - Tracks P (peak) and T (trough) of the pulse waveform each beat.
 *   - Adaptive threshold = 50 % of the P–T amplitude.
 *   - IBI = milliseconds between consecutive upward threshold-crossings.
 *   - BPM = 60000 / (rolling average of last 10 IBI values).
 *
 * Zephyr / MAX30102 adaptation notes:
 *   ┌─────────────────────────────────────────────────────────────────┐
 *   │ Arduino             │ This file                                 │
 *   ├─────────────────────┼───────────────────────────────────────────┤
 *   │ Timer2 ISR          │ k_timer + k_work (ppg_work_handler)       │
 *   │ analogRead()        │ sensor_sample_fetch / sensor_channel_get  │
 *   │ 500 Hz / 2 ms step  │ HR_SAMPLE_RATE Hz / MS_PER_SAMPLE ms step │
 *   │ 10-bit ADC 0–1023   │ 18-bit IR 0–262143; seeds scaled to       │
 *   │   P/T seed = 512    │   P/T seed = HR_MIN_VALID_IR              │
 *   │   thresh seed = 530 │   thresh seed = HR_MIN_VALID_IR + 500     │
 *   │ N > 250 samples     │ N > 250 ms  (unit is already ms in both)  │
 *   │ N > 2500 samples    │ N > 2500 ms (same)                        │
 *   └─────────────────────┴───────────────────────────────────────────┘
 *
 *  sampleCounter / lastBeatTime are tracked in **milliseconds** in both
 *  versions, so all timing comparisons (250, 2500, IBI/5*3 …) transfer
 *  unchanged.  Only the seed/reset amplitude values are rescaled.
 */

#define DT_DRV_COMPAT maxim_max30102

#include "PPG.hpp"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(MAX30102, CONFIG_SENSOR_LOG_LEVEL);

/* ── Timing ─────────────────────────────────────────────────────────────── */

/* Milliseconds elapsed per sample (5 ms at 200 Hz). */
#define MS_PER_SAMPLE   (1000 / HR_SAMPLE_RATE)

/* ── IBI Seed / Reset Values (scaled for 18-bit MAX30102 IR) ────────────── */

/*
 * Arduino seeded P and T at the ADC mid-point (512 / 1023).
 * For MAX30102 we use HR_MIN_VALID_IR as a conservative "no-signal" level.
 * The adaptive threshold self-corrects within the first beat cycle.
 */
#define IBI_SEED_MIDPOINT   HR_MIN_VALID_IR          /* ~midpoint with no finger */
#define IBI_SEED_THRESH     (HR_MIN_VALID_IR + 500)  /* just above mid at reset  */

/* ── IBI State (mirrors Arduino volatile globals) ───────────────────────── */

static int32_t  ibi_rate[10];          /* last 10 IBI values (ms)            */
static uint32_t sample_counter = 0;    /* running ms clock                   */
static uint32_t last_beat_time = 0;    /* ms timestamp of last detected beat */
static int32_t  P_val;                 /* peak of current pulse waveform     */
static int32_t  T_val;                 /* trough of current pulse waveform   */
static int32_t  thresh;                /* adaptive beat-detection threshold  */
static int32_t  amp;                   /* P–T amplitude of current beat      */
static bool     first_beat  = true;    /* startup: first crossing, skip IBI  */
static bool     second_beat = false;   /* startup: seed rate[] with 1st IBI  */
static bool     pulse_flag  = false;   /* true while signal is above thresh  */
static int32_t  ibi         = 600;     /* current IBI (ms), seeded at 100 BPM*/

/* ── Output State ───────────────────────────────────────────────────────── */

/* Latest valid BPM (−1 = not yet available). */
static int32_t  latest_bpm       = -1;

/*
 * Count of consecutive valid BPM readings (0–10).
 * Used by ppg_get_confidence() to indicate warm-up progress.
 */
static uint8_t  valid_bpm_count  = 0;

/* ── Shared Sensor Handle ────────────────────────────────────────────────── */

static struct sensor_value ir_val;
const struct device *dev = DEVICE_DT_GET_ANY(maxim_max30102);

/* ══════════════════════════════════════════════════════════════════════════
 * DRIVER LAYER  (sample_fetch / channel_get / init / DT macro)
 * ══════════════════════════════════════════════════════════════════════════ */

static int max30102_sample_fetch(const struct device *dev,
                                 enum sensor_channel chan)
{
    struct max30102_data *data = (max30102_data *)dev->data;
    const struct max30102_config *cfg  = (const struct max30102_config *)dev->config;

    uint8_t buffer[MAX30102_MAX_NUM_CHANNELS * MAX30102_BYTES_PER_CHANNEL];

    if (i2c_burst_read_dt(&cfg->i2c, MAX30102_REG_FIFO_DATA,
                          buffer, sizeof(buffer))) {
        LOG_ERR("Failed to read FIFO");
        return -EIO;
    }

    /* Unpack RED (bytes 0–2) */
    data->raw[MAX30102_LED_CHANNEL_RED] =
        ((uint32_t)buffer[0] << 16) |
        ((uint32_t)buffer[1] <<  8) |
         (uint32_t)buffer[2];
    data->raw[MAX30102_LED_CHANNEL_RED] &= MAX30102_FIFO_DATA_MASK;

    /* Unpack IR (bytes 3–5) */
    data->raw[MAX30102_LED_CHANNEL_IR] =
        ((uint32_t)buffer[3] << 16) |
        ((uint32_t)buffer[4] <<  8) |
         (uint32_t)buffer[5];
    data->raw[MAX30102_LED_CHANNEL_IR] &= MAX30102_FIFO_DATA_MASK;

    /* Clear interrupt status */
    uint8_t status;
    i2c_reg_read_byte_dt(&cfg->i2c, MAX30102_REG_INT_STS1, &status);

    /*
     * Store into circular IR buffer (kept for diagnostics / future SpO2 use).
     * The IBI algorithm processes samples one-at-a-time in ppg_read(), so
     * ir_buf_full is informational only — BPM does not wait for it.
     */
    data->ir_buffer[data->ir_buf_idx] = data->raw[MAX30102_LED_CHANNEL_IR];
    data->ir_buf_idx++;
    if (data->ir_buf_idx >= HR_BUFFER_SIZE) {
        data->ir_buf_idx  = 0;
        data->ir_buf_full = true;
    }

    return 0;
}

static int max30102_channel_get(const struct device *dev,
                                enum sensor_channel chan,
                                struct sensor_value *val)
{
    struct max30102_data *data = (max30102_data *)dev->data;

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

static const struct sensor_driver_api max30102_driver_api = {
    .sample_fetch = max30102_sample_fetch,
    .channel_get  = max30102_channel_get,
};

static int max30102_init(const struct device *dev)
{
    const struct max30102_config *cfg = (struct max30102_config *)dev->config;
    struct max30102_data *data        = (struct max30102_data *)dev->data;
    uint8_t part_id, mode_cfg;

    memset(data, 0, sizeof(*data));

    if (!device_is_ready(cfg->i2c.bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    if (i2c_reg_read_byte_dt(&cfg->i2c, MAX30102_REG_PART_ID, &part_id)) {
        LOG_ERR("Could not read Part ID");
        return -EIO;
    }
    if (part_id != MAX30102_PART_ID) {
        LOG_ERR("Wrong Part ID: 0x%02x (expected 0x%02x)",
                part_id, MAX30102_PART_ID);
        return -EIO;
    }

    /* Reset the device and wait for the bit to self-clear. */
    if (i2c_reg_write_byte_dt(&cfg->i2c, MAX30102_REG_MODE_CFG,
                               MAX30102_MODE_CFG_RESET_MASK)) {
        return -EIO;
    }
    do {
        if (i2c_reg_read_byte_dt(&cfg->i2c,
                                  MAX30102_REG_MODE_CFG, &mode_cfg)) {
            return -EIO;
        }
    } while (mode_cfg & MAX30102_MODE_CFG_RESET_MASK);

    if (i2c_reg_write_byte_dt(&cfg->i2c, MAX30102_REG_FIFO_CFG, cfg->fifo) ||
        i2c_reg_write_byte_dt(&cfg->i2c, MAX30102_REG_MODE_CFG, cfg->mode) ||
        i2c_reg_write_byte_dt(&cfg->i2c, MAX30102_REG_SPO2_CFG, cfg->spo2) ||
        i2c_reg_write_byte_dt(&cfg->i2c, MAX30102_REG_LED1_PA,  cfg->led_pa[0]) ||
        i2c_reg_write_byte_dt(&cfg->i2c, MAX30102_REG_LED2_PA,  cfg->led_pa[1])) {
        return -EIO;
    }

    LOG_INF("MAX30102 initialized");
    printk("MAX30102: Ready. Place finger on sensor...\n");
    return 0;
}

#define MAX30102_DEFINE(inst)                                                   \
    static struct max30102_data max30102_data_##inst;                           \
                                                                                \
    static const struct max30102_config max30102_config_##inst = {              \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                     \
        .fifo = (DT_INST_PROP(inst, smp_ave)                                    \
                    << MAX30102_FIFO_CFG_SMP_AVE_SHIFT) |                       \
                COND_CODE_1(DT_INST_PROP(inst, fifo_rollover_en),               \
                    (MAX30102_FIFO_CFG_ROLLOVER_MASK |), ())                    \
                (DT_INST_PROP(inst, fifo_a_full)                                \
                    << MAX30102_FIFO_CFG_FULL_SHIFT),                           \
        .mode = DT_INST_PROP(inst, mode),                                       \
        .spo2 = (DT_INST_PROP(inst, adc_rge)                                    \
                    << MAX30102_SPO2_ADC_RGE_SHIFT) |                           \
                (DT_INST_PROP(inst, sr)  << MAX30102_SPO2_SR_SHIFT) |           \
                (MAX30102_PW_18BITS      << MAX30102_SPO2_PW_SHIFT),            \
        .led_pa = { DT_INST_PROP(inst, led1_pa),                                \
                    DT_INST_PROP(inst, led2_pa) },                              \
    };                                                                          \
                                                                                \
    DEVICE_DT_INST_DEFINE(inst, max30102_init, NULL,                            \
                          &max30102_data_##inst, &max30102_config_##inst,       \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,             \
                          &max30102_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX30102_DEFINE)

/* ══════════════════════════════════════════════════════════════════════════
 * APPLICATION LAYER  (IBI beat detection, BPM averaging)
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * ibi_reset() - Restore IBI state variables to power-on defaults.
 *
 * Called at startup (ppg_init) and whenever 2.5 s pass with no beat.
 * Mirrors the Arduino ISR's "N > 2500" recovery block.
 */
static void ibi_reset(void)
{
    thresh          = IBI_SEED_THRESH;
    P_val           = IBI_SEED_MIDPOINT;
    T_val           = IBI_SEED_MIDPOINT;
    last_beat_time  = sample_counter;
    first_beat      = true;
    second_beat     = false;
    pulse_flag      = false;
    latest_bpm      = -1;
    valid_bpm_count = 0;
}

/* ── Work / Timer Plumbing ───────────────────────────────────────────────── */

static struct k_work ppg_work;

static void ppg_work_handler(struct k_work *work)
{
    ppg_read();
}

void PPG_handler(struct k_timer *timer_id)
{
    if (!k_work_is_pending(&ppg_work)) {
        k_work_submit(&ppg_work);
    }
}

K_TIMER_DEFINE(ppg_timer, PPG_handler, NULL);

/* ── Public API ─────────────────────────────────────────────────────────── */

int ppg_init(int sample_rate_hz)
{
    if (!device_is_ready(dev)) {
        printk("MAX30102 not ready\n");
        return -1;
    }

    ir_val.val1 = 0;
    ir_val.val2 = 0;

    /* Initialise IBI tracking state. */
    sample_counter = 0;
    ibi            = 600;           /* 100 BPM seed — overwritten after 1st beat */
    for (int i = 0; i < 10; i++) {
        ibi_rate[i] = ibi;
    }
    ibi_reset();

    uint32_t period_us = 1000000U / (uint32_t)sample_rate_hz;
    k_work_init(&ppg_work, ppg_work_handler);
    k_timer_start(&ppg_timer, K_USEC(period_us), K_USEC(period_us));

    printk("PPG: IBI mode, %d Hz (%d ms/sample), buffer=%d samples\n",
           sample_rate_hz, MS_PER_SAMPLE, HR_BUFFER_SIZE);
    return 0;
}

static bool ibi_seeded = false;

/*
 * ppg_read() - Called every MS_PER_SAMPLE ms by the work handler.
 *
 * Implements the PulseSensor IBI algorithm sample-by-sample, translated
 * from the Arduino Timer2 ISR.  Variable names are kept close to the
 * original (P→P_val, T→T_val, Pulse→pulse_flag, IBI→ibi, N→N) to make
 * side-by-side comparison straightforward.
 *
 * Signal path per call:
 *   1. Fetch one IR sample from the MAX30102.
 *   2. Finger-presence guard — bail out if IR is below HR_MIN_VALID_IR.
 *   3. Advance the ms clock (sample_counter).
 *   4. Track trough (T_val) in the falling portion of the wave.
 *   5. Track peak (P_val) in the rising portion.
 *   6. Upward threshold-crossing → beat detected:
 *        a. Compute IBI.
 *        b. Seed rate[] on second beat; discard IBI on first beat.
 *        c. Roll the 10-element rate array, compute average, derive BPM.
 *   7. Downward threshold-crossing → beat over:
 *        a. Compute amp = P_val − T_val.
 *        b. Set new thresh at 50 % of amp (adaptive threshold).
 *   8. 2.5 s timeout without a beat → full IBI reset.
 */
void ppg_read(void)
{
    if (sensor_sample_fetch(dev) < 0) {
        printk("MAX30102 fetch failed\n");
        return;
    }

    sensor_channel_get(dev, SENSOR_CHAN_IR, &ir_val);

    int32_t signal = (int32_t)ir_val.val1;

    /* ── 2. Finger-presence guard ── */
    if (signal < HR_MIN_VALID_IR) {
        /*
         * No finger on sensor.  Hold state frozen; if the absence lasts
         * long enough the 2.5 s timeout below will perform a full reset.
         */
        sample_counter += MS_PER_SAMPLE;
        int32_t N = (int32_t)(sample_counter - last_beat_time);
        if (N > 2500) {
            printk("PPG: no finger / no beat for 2.5 s — resetting\n");
            ibi_reset();
        }
        return;
    }

    if (!ibi_seeded) {
        P_val       = signal;
        T_val       = signal;
        thresh      = signal;
        ibi_seeded  = true;
        sample_counter += MS_PER_SAMPLE;
        last_beat_time  = sample_counter;
        return;
    }

    /* ── 3. Advance ms clock ── */
    sample_counter += MS_PER_SAMPLE;
    int32_t N = (int32_t)(sample_counter - last_beat_time);

    /* ── 4. Track trough ──
     *
     * Only update T_val when we are below thresh and past 3/5 of the last
     * IBI — same dichrotic-notch avoidance as the Arduino original.
     */
    if (signal < thresh && N > (ibi / 5) * 3) {
        if (signal < T_val) {
            T_val = signal;
        }
    }

    /* ── 5. Track peak ── */
    if (signal > thresh && signal > P_val) {
        P_val = signal;
    }

    /* ── 6. Upward threshold-crossing → beat detected ──────────────────── */
    if (N > 250) {   /* 250 ms minimum — rejects >240 BPM noise */
        if (signal > thresh && !pulse_flag && N > (ibi / 5) * 3) {

            pulse_flag     = true;
            ibi            = (int32_t)(sample_counter - last_beat_time);
            last_beat_time = sample_counter;

            /* ── 6b. Startup seeding ── */
            if (second_beat) {
                second_beat = false;
                /* Fill all 10 slots with the first real IBI so the
                 * running average starts at a plausible value. */
                for (int i = 0; i < 10; i++) {
                    ibi_rate[i] = ibi;
                }
            }

            if (first_beat) {
                /* First crossing is always noisy — discard IBI, arm
                 * second_beat flag, and return without updating BPM. */
                first_beat  = false;
                second_beat = true;
                return;
            }

            /* ── 6c. Rolling average of last 10 IBI values → BPM ── */
            int32_t running_total = 0;
            for (int i = 0; i < 9; i++) {
                ibi_rate[i]    = ibi_rate[i + 1]; /* shift left, drop oldest */
                running_total += ibi_rate[i];
            }
            ibi_rate[9]    = ibi;           /* append newest IBI        */
            running_total += ibi_rate[9];
            running_total /= 10;            /* average of 10 IBI values */

            int32_t bpm = 60000 / running_total;

            if (bpm >= HR_MIN_BPM && bpm <= HR_MAX_BPM) {
                latest_bpm = bpm;
                if (valid_bpm_count < 10) {
                    valid_bpm_count++;
                }
                printk("PPG: IBI=%d ms  BPM=%d\n", (int)ibi, (int)bpm);
            } else {
                printk("PPG: BPM %d out of range [%d–%d], discarding\n",
                       (int)bpm, HR_MIN_BPM, HR_MAX_BPM);
            }
        }
    }

    /* ── 7. Downward threshold-crossing → beat is over ────────────────── */
    if (signal < thresh && pulse_flag) {
        pulse_flag = false;

        amp    = P_val - T_val;          /* amplitude of this pulse          */
        thresh = amp / 2 + T_val;        /* new thresh at 50 % of amplitude  */
        P_val  = thresh;                 /* reset peak for next beat         */
        T_val  = thresh;                 /* reset trough for next beat       */
    }

    /* ── 8. 2.5 s timeout — no beat detected ───────────────────────────── */
    if (N > 2500) {
        printk("PPG: no beat for 2.5 s — resetting IBI state\n");
        ibi_reset();
    }
}

/* ── Accessors ──────────────────────────────────────────────────────────── */

struct sensor_value *ppg_get_data(void) { return &ir_val; }

int32_t ppg_get_bpm(void) { return latest_bpm; }

/*
 * ppg_get_confidence() - 0–100 warm-up confidence indicator.
 *
 * Returns the percentage of the 10-reading IBI history that has been
 * populated with real (range-validated) BPM values since the last reset.
 * 0 = just started / no finger; 100 = fully converged.
 */
int32_t ppg_get_confidence(void)
{
    return (int32_t)valid_bpm_count * 10;  /* 0, 10, 20 … 100 */
}