#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <math.h>
#include "ADC.h"
#include "LSM6DS3TR.h"
#include "PPG.h"

#define EMG_FREQUENCY   4000
#define EMG_PERIOD      K_USEC(1000000 / EMG_FREQUENCY)

/* Variables for sampling */
int32_t emg_window[EMG_WINDOW_SIZE] = {0}; // cirucular buffer
emg_metrics_t emg_data;
static int32_t emg_idx;
int32_t emg_raw = 0;

/* Variables for ADC init*/
static uint16_t  channel_reading[SEQUENCE_SAMPLES]; // raw ADC values

static const struct adc_sequence_options options = {
    .extra_samplings = SEQUENCE_SAMPLES - 1,
    .interval_us     = 0,
};
static struct adc_sequence sequence = {
    .buffer      = channel_reading,
    .buffer_size = sizeof(channel_reading),
    .resolution  = SEQUENCE_RESOLUTION,
    .options     = &options,
    .channels    = 0,
};

static const struct device *adc = DEVICE_DT_GET(ADC_NODE);
static const struct adc_channel_cfg channel_cfgs[] = {
    DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))};
static uint32_t vrefs_mv[] = {
    DT_FOREACH_CHILD_SEP(ADC_NODE, CHANNEL_VREF, (,))};

/* Periodic timer handler */ 
static struct k_work emg_work;

static void EMG_handler(struct k_timer *timer_id)
{
    k_work_submit(&emg_work);
}

static void emg_work_handler(struct k_work *work)
{
    EMG_read();
}

K_TIMER_DEFINE(emg_timer, EMG_handler, NULL);

/* Integer square root using the Babylonian method */
static uint32_t isqrt32(uint32_t n)
{
    if (n == 0) return 0;
    uint32_t x = n, y = 1;
    while (x > y) { 
        x = (x + y) / 2; 
        y = n / x; 
    }
    return x;
}



int ADC_init(void)
{
    int err;
    if (!device_is_ready(adc)) {
        printk("ADC controller device %s not ready\n", adc->name);
        return -1;
    }

    /* Setup the channel */
    sequence.channels = BIT(channel_cfgs[0].channel_id);
    err = adc_channel_setup(adc, &channel_cfgs[0]);
    if (err < 0) {
        printk("Could not setup channel (%d)\n", err);
        return -1;
    }

    if ((vrefs_mv[0] == 0) && (channel_cfgs[0].reference == ADC_REF_INTERNAL)) {
        vrefs_mv[0] = adc_ref_internal(adc);
    }

    for (int i = 0; i < EMG_WINDOW_SIZE; ++i) {
        emg_window[i] = 0;
    }
    emg_idx = 0;

    /* Initialize timer */
    k_work_init(&emg_work, emg_work_handler);
    k_timer_start(&emg_timer, EMG_PERIOD, EMG_PERIOD);

    return 0;
}

/* Read data from ADC. Takes 64 samples to reduce noise. 
*/
void EMG_read() {
    int err;

    // read from ADC
    err = adc_read(adc, &sequence);
    if (err < 0) {
        printk("Could not read (%d)\n", err);
        return;
    }
    emg_raw = channel_reading[0];
    err = adc_raw_to_millivolts(vrefs_mv[0], channel_cfgs[0].gain, SEQUENCE_RESOLUTION, &emg_raw);

    // store into window
    emg_window[emg_idx] = emg_raw;
    emg_idx = (emg_idx + 1) & (EMG_WINDOW_SIZE - 1);
}

int32_t *EMG_get_raw() { return &emg_raw; }

/* Integer square root — already in file, reused below */

/* Compute EMG metrics over the last EMG_ANALYSIS_SAMPLES entries
 * in the circular emg_window buffer.
 *
 * emg_window is filled by EMG_read()() at ADC_FREQUENCY (4 kHz).
 * Each entry is already in mV (averaged over SEQUENCE_SAMPLES raw reads).
 * emg_idx points to the NEXT write slot, so reading backwards from there
 * gives the most recent data.
 *
 * Returns 0 on success, -EINVAL if out is NULL,
 * -EAGAIN if the buffer hasn't been filled enough yet.
 */
int EMG_compute_from_window()
{
    /* Reject if we haven't accumulated enough samples yet.
     * emg_idx wraps, so compare against a sentinel: once emg_idx
     * has lapped the ring we know the buffer is full. Use a simple
     * static latch instead of a separate flag. */
    static bool window_ready = false;
    if (!window_ready) {
        /* Once idx has advanced past EMG_ANALYSIS_SAMPLES we have
         * enough data regardless of wrap position.              */
        if (emg_idx >= EMG_ANALYSIS_SAMPLES) {
            window_ready = true;
        } else {
            return -EAGAIN;
        }
    }

    /* ── Pass 1: mean (DC offset) ─────────────────────────────────── */
    int64_t sum = 0;
    int32_t min_mv = INT32_MAX;
    int32_t max_mv = INT32_MIN;

    /* Start index of the oldest sample in the analysis window */
    uint32_t start = (emg_idx + EMG_WINDOW_SIZE - EMG_ANALYSIS_SAMPLES)
                      % EMG_WINDOW_SIZE;

    for (uint32_t n = 0; n < EMG_ANALYSIS_SAMPLES; n++) {
        uint32_t idx = (start + n) % EMG_WINDOW_SIZE;
        int32_t v = emg_window[idx];

        sum += v;
        if (v < min_mv) min_mv = v;
        if (v > max_mv) max_mv = v;
    }

    int32_t dc = (int32_t)(sum / EMG_ANALYSIS_SAMPLES);
    emg_data.dc_offset_mv = dc;
    emg_data.peak_to_peak_mv = max_mv - min_mv;

    /* ── Pass 2: AC metrics (MAV, RMS, ZCR) ──────────────────────── */
    int64_t sum_abs = 0;
    int64_t sum_sq  = 0;
    uint16_t zcr = 0;

    /* Seed prev_ac from the first sample */
    int32_t prev_ac = emg_window[start] - dc;

    for (uint32_t n = 0; n < EMG_ANALYSIS_SAMPLES; n++) {
        uint32_t idx = (start + n) % EMG_WINDOW_SIZE;
        int32_t  ac  = emg_window[idx] - dc;

        sum_abs += (ac < 0) ? -ac : ac;
        sum_sq += (int64_t)ac * ac;

        if (n > 0) {
            if ((prev_ac < 0 && ac >= 0) || (prev_ac >= 0 && ac < 0)) {
                zcr++;
            }
        }
        prev_ac = ac;
    }

    emg_data.mav_mv = (int32_t)(sum_abs / EMG_ANALYSIS_SAMPLES);
    emg_data.rms_mv = (int32_t)isqrt32((uint32_t)(sum_sq / EMG_ANALYSIS_SAMPLES));
    emg_data.zcr = zcr;
    emg_data.is_active = (emg_data.rms_mv > EMG_ACTIVATION_THRESHOLD_MV);
    emg_data.fatigue_flag = emg_data.is_active && (emg_data.zcr < EMG_FATIGUE_ZCR_LOW);

    return 0;
}

emg_metrics_t *EMG_get_metrics() { return &emg_data; }