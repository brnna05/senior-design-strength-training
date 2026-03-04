#ifndef _ADC_H_
#define _ADC_H_

#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

/* ── Sampling config ─────────────────────────────────────────────────────── */
#define SEQUENCE_SAMPLES      64    /* power-of-2 window for EMG analysis     */
#define SEQUENCE_RESOLUTION   12

/* ── ADC device-tree ─────────────────────────────────────────────────────── */
#define ADC_NODE        DT_ALIAS(adc0)
#define CHANNEL_VREF(node_id) DT_PROP_OR(node_id, zephyr_vref_mv, 0)
#define CHANNEL_COUNT 1

/* ── EMG activation thresholds ───────────────────────────────────────────── */
/* Tune these to your electrode placement / gain stage */
#define EMG_ACTIVATION_THRESHOLD_MV   20   /* RMS above this = muscle active  */
#define EMG_FATIGUE_ZCR_LOW           10   /* crossings/window below this = fatigue indicator */

#define ADC_ON 1
#define PPG_ON 0
#define IMU_ON 1
#define BLE_ON 0

/* ── EMG result struct ───────────────────────────────────────────────────── */
/**
 * @brief Holds all per-window EMG metrics derived from a differential
 *        bipolar electrode pair placed over the biceps brachii.
 *
 * Interpretation guide
 * --------------------
 * peak_to_peak_mv  – gross signal amplitude; correlates with motor-unit
 *                    recruitment (more fibres firing → larger swing).
 * rms_mv           – true power estimate; standard "activation level" metric.
 *                    Use for force/contraction-intensity estimation.
 * mav_mv           – mean absolute value; computationally cheap alternative
 *                    to RMS, very common in prosthetics & HCI.
 * zcr              – zero-crossing rate; rough spectral index.
 *                    Fresh muscle → higher ZCR (~50-150 Hz median freq).
 *                    Fatiguing muscle → ZCR drops as MPF shifts downward.
 * is_active        – simple threshold gate: TRUE when RMS > threshold.
 * fatigue_flag     – heuristic: active but ZCR below expected range.
 * dc_offset_mv     – any residual DC; should be ~0 with good differential
 *                    electrodes; large offset hints at poor contact / motion.
 */
typedef struct {
    int32_t  peak_to_peak_mv;   /* max - min of window [mV]                  */
    int32_t  rms_mv;            /* sqrt( mean(x²) ) [mV]                     */
    int32_t  mav_mv;            /* mean( |x| ) [mV]                          */
    uint16_t zcr;               /* zero-crossing count in window             */
    int32_t  dc_offset_mv;      /* mean value (bias) [mV]                    */
    bool     is_active;         /* RMS > EMG_ACTIVATION_THRESHOLD_MV         */
    bool     fatigue_flag;      /* active + low ZCR heuristic                */
} emg_metrics_t;

/* ── Public API ──────────────────────────────────────────────────────────── */

/* Initializes ADC for channels in overlay */
int ADC_init(void);

/**
 * Reads one window of SEQUENCE_SAMPLES, converts to mV, and returns
 * the populated emg_metrics_t.  Returns 0 on success, negative on error.
 */
int EMG_read(emg_metrics_t *out);

/* Read EMG metrics and print */
void EMG_print(void);

/* Reads the data from the channel and displays it */
void ADC_print();

#endif /* _ADC_H_ */