#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <math.h>
#include "ADC.h"

#define ADC_FREQUENCY   4000
#define ADC_PERIOD      K_USEC(1000000 / ADC_FREQUENCY)

static uint16_t  channel_reading[SEQUENCE_SAMPLES];  // raw ADC values
static int32_t   window_mv[SEQUENCE_SAMPLES]; // converted to mV for metric calculations

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

// Periodic timer handlers
static struct k_work adc_work;

static void ADC_handler(struct k_timer *timer_id)
{
    k_work_submit(&adc_work);
}

static void adc_work_handler(struct k_work *work)
{
    EMG_print();
}

K_TIMER_DEFINE(adc_timer, ADC_handler, NULL);

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

/* Initialises ADC hardware and starts the periodic sampling timer. */
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

    /* Initialize timer */
    k_work_init(&adc_work, adc_work_handler);
    k_timer_start(&adc_timer, ADC_PERIOD, ADC_PERIOD);

    return 0;
}

/* Samples ADC and computes EMG metrics
 *   dc_offset_mv   – mean(window)          [mV]
 *   peak_to_peak_mv– max(window)-min        [mV]
 *   mav_mv         – mean(|AC sample|)      [mV]  (DC removed first)
 *   rms_mv         – sqrt(mean(AC sample²)) [mV]
 *   zcr            – count of sign changes in AC window
 *   is_active      – rms_mv > threshold
 *   fatigue_flag   – active + ZCR suspiciously low
 */
int EMG_read(emg_metrics_t *out)
{
    if (!out) return -EINVAL;

    // sample adc
    int err = adc_read(adc, &sequence);
    if (err < 0) {
        printk("ADC read error (%d)\n", err);
        return err;
    }

    // convert to mV and compute min/max in one pass
    int32_t min_mv = INT32_MAX, max_mv = INT32_MIN;
    int64_t sum = 0;

    for (size_t i = 0; i < SEQUENCE_SAMPLES; i++) {
        int32_t v = (int32_t)channel_reading[i];
                    err = adc_raw_to_millivolts(vrefs_mv[0],
                    channel_cfgs[0].gain,
                    SEQUENCE_RESOLUTION,
                    &v);
        if (err < 0 || vrefs_mv[0] == 0) {
            printk("mV conversion unsupported\n");
            return -ENOTSUP;
        }
        window_mv[i] = v;
        sum += v;
        if (v < min_mv) min_mv = v;
        if (v > max_mv) max_mv = v;
    }

    // DC offset
    int32_t dc = (int32_t)(sum / SEQUENCE_SAMPLES);
    out->dc_offset_mv    = dc;
    out->peak_to_peak_mv = max_mv - min_mv;

    // AC metrics
    int64_t  sum_abs  = 0;
    int64_t  sum_sq   = 0;
    uint16_t zcr      = 0;
    int32_t  prev_ac  = window_mv[0] - dc;

    for (size_t i = 0; i < SEQUENCE_SAMPLES; i++) {
        int32_t ac = window_mv[i] - dc;

        sum_abs += (ac < 0) ? -ac : ac;

        sum_sq  += (int64_t)ac * ac;

        if (i > 0) {
            if ((prev_ac < 0 && ac >= 0) || (prev_ac >= 0 && ac < 0)) {
                zcr++;
            }
        }
        prev_ac = ac;
    }

    out->mav_mv    = (int32_t)(sum_abs / SEQUENCE_SAMPLES);
    out->rms_mv    = (int32_t)isqrt32((uint32_t)(sum_sq / SEQUENCE_SAMPLES));
    out->zcr       = zcr;
    out->is_active    = (out->rms_mv > EMG_ACTIVATION_THRESHOLD_MV);
    out->fatigue_flag = out->is_active && (out->zcr < EMG_FATIGUE_ZCR_LOW);

    return 0;
}

/* Read EMG metrics and print */
void EMG_print(void)
{
    emg_metrics_t m;
    int64_t ts = k_uptime_get();

    if (EMG_read(&m) < 0) return;

    printk("ts=%lld  p2p=%dmV  rms=%dmV  mav=%dmV  zcr=%u  dc=%dmV  "
           "active=%d  fatigue=%d\n",
           ts,
           m.peak_to_peak_mv,
           m.rms_mv,
           m.mav_mv,
           m.zcr,
           m.dc_offset_mv,
           (int)m.is_active,
           (int)m.fatigue_flag);
}

void ADC_print() {
    int err;
    int64_t timestamp_us = k_ticks_to_us_floor64(k_uptime_ticks());

    err = adc_read(adc, &sequence);
    if (err < 0) {
        printk("Could not read (%d)\n", err);
        return;
    }

    int32_t val_mv = 0;
    int sum = 0;
        
    for (size_t i = 0U; i < SEQUENCE_SAMPLES; i++) {

        val_mv = channel_reading[i];
        err = adc_raw_to_millivolts(vrefs_mv[0], channel_cfgs[0].gain, SEQUENCE_RESOLUTION, &val_mv);
        /* conversion to mV may not be supported, skip if not */
	    if (!((err < 0) || vrefs_mv[0] == 0)) {
            sum += val_mv;
	    }
    }

    val_mv = sum / SEQUENCE_SAMPLES;

    printk("%lld, %d\n", timestamp_us, val_mv);
}