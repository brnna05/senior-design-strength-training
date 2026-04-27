/*
 * classifier.c — Rep Detection & Fatigue Classification
 * Senior Design - Strength Training
 *
 * Signals used:
 *   EMG (via ADC.h):  rms_mv, mav_mv, zcr, is_active, fatigue_flag
 *   PPG (via PPG.hpp): ppg_get_bpm()
 *   IMU:              NOT USED (hardware unavailable on PCB)
 */

#include "classifier.h"
#include "ADC.h"    /* EMG_compute_from_window(), EMG_get_metrics()  */
#include "PPG.hpp"  /* ppg_get_bpm()                                 */

#include <zephyr/sys/printk.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

/* ══════════════════════════════════════════════════════════════════════════
 * Tunables
 * ══════════════════════════════════════════════════════════════════════════ */

/* How often classifier_update() is called (ms). Must match the timer in main. */
#define CLASSIFIER_PERIOD_MS        100U

/* Rep detection ---------------------------------------------------------- */

/* Minimum EMG activation duration to count as a rep (ms).
 * Filters out noise / involuntary twitches.                    */
#define REP_MIN_ACTIVE_MS           300U

/* Mandatory rest between consecutive reps (ms).
 * Prevents double-counting from brief EMG drop-outs mid-rep.   */
#define REP_COOLDOWN_MS             500U
#define REP_MAX_ACTIVE_MS           3000U

/* Per-rep EMG snapshot history ------------------------------------------ */

/* How many recent reps to track for trend analysis. */
#define REP_HISTORY_SIZE            5

/* Fatigue thresholds ----------------------------------------------------- */

/* Heart rate bands (BPM) */
#define HR_MILD_THRESHOLD           120   /* HR ≥ this adds 1 hr_score pt   */
#define HR_MODERATE_THRESHOLD       150   /* HR ≥ this adds 2 hr_score pts  */

/* ZCR % drop over the rep history that indicates median-frequency shift.
 * Muscle fatigue causes the EMG power spectrum to shift to lower
 * frequencies, which manifests as a declining zero-crossing rate.        */
#define ZCR_FATIGUE_DROP_PCT        25    /* 25% drop → trend score += 1    */

/* Minimum reps in history before we trust the trend. */
#define ZCR_TREND_MIN_REPS          3

/* ══════════════════════════════════════════════════════════════════════════
 * Rep FSM
 * ══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    REP_STATE_IDLE,      /* Muscle at rest between reps                   */
    REP_STATE_ACTIVE,    /* EMG activation detected — rep in progress      */
    REP_STATE_COOLDOWN,  /* Brief mandatory rest after rep was counted      */
} rep_state_t;

static rep_state_t rep_state      = REP_STATE_IDLE;
static uint32_t    state_ticks    = 0;   /* ticks spent in current state    */

/* ══════════════════════════════════════════════════════════════════════════
 * Per-rep EMG snapshot ring buffer
 * ══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    int32_t  rms_mv;
    int32_t  mav_mv;
    uint16_t zcr;
} rep_snapshot_t;

static rep_snapshot_t rep_history[REP_HISTORY_SIZE];
static uint8_t        rep_history_head  = 0;   /* next write slot (circular) */
static uint8_t        rep_history_count = 0;   /* valid entries (0–REP_HISTORY_SIZE) */

static void push_rep_snapshot(const emg_metrics_t *m)
{
    rep_history[rep_history_head].rms_mv = m->rms_mv;
    rep_history[rep_history_head].mav_mv = m->mav_mv;
    rep_history[rep_history_head].zcr    = m->zcr;
    rep_history_head = (rep_history_head + 1) % REP_HISTORY_SIZE;
    if (rep_history_count < REP_HISTORY_SIZE) {
        rep_history_count++;
    }
}

/* Return the snapshot that is `offset` steps back from the most recent.
 * offset=0 → newest, offset=(rep_history_count-1) → oldest.              */
static const rep_snapshot_t *history_at(uint8_t offset)
{
    uint8_t idx = (rep_history_head + REP_HISTORY_SIZE - 1 - offset)
                  % REP_HISTORY_SIZE;
    return &rep_history[idx];
}

static void reset_rep_history(void)
{
    rep_history_head  = 0;
    rep_history_count = 0;
}

/* ══════════════════════════════════════════════════════════════════════════
 * Fatigue Scoring
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * compute_emg_score() — 0, 1, or 2.
 *
 * 2 → fatigue_flag set in the most recent EMG metrics window.
 *     (muscle is active AND ZCR is below the configured low threshold —
 *      this is the strongest single-sample fatigue indicator.)
 *
 * 1 → ZCR has declined ≥ ZCR_FATIGUE_DROP_PCT% across the last
 *     ZCR_TREND_MIN_REPS reps without the flag being set.  This catches
 *     accumulating fatigue before it crosses the hard threshold.
 *
 * 0 → No evidence of fatigue in current or recent data.
 */
static int compute_emg_score(const emg_metrics_t *emg)
{
    /* Strongest signal: fatigue_flag already computed in ADC.c */
    if (emg->fatigue_flag) {
        return 2;
    }

    /* Trend signal: ZCR declining across reps */
    if (rep_history_count >= ZCR_TREND_MIN_REPS) {
        const rep_snapshot_t *newest = history_at(0);
        const rep_snapshot_t *oldest = history_at(rep_history_count - 1);

        if (oldest->zcr > 0) {
            int drop_pct = (int)(oldest->zcr - newest->zcr) * 100
                           / (int)oldest->zcr;
            if (drop_pct >= ZCR_FATIGUE_DROP_PCT) {
                printk("CLASSIFIER: ZCR trend drop=%d%% → EMG score 1\n",
                       drop_pct);
                return 1;
            }
        }
    }

    return 0;
}

/*
 * compute_hr_score() — 0, 1, or 2.
 *
 * 2 → HR ≥ HR_MODERATE_THRESHOLD (high cardiovascular load)
 * 1 → HR ≥ HR_MILD_THRESHOLD     (moderate effort)
 * 0 → HR < threshold or no finger present
 *
 * Note: Both scores require EMG activity to contribute meaningfully to
 * the final fatigue level (see compute_fatigue).
 */
static int compute_hr_score(int32_t bpm)
{
    if (bpm < 0) {
        return 0;   /* no finger / not yet converged */
    }
    if (bpm >= HR_MODERATE_THRESHOLD) {
        return 2;
    }
    if (bpm >= HR_MILD_THRESHOLD) {
        return 1;
    }
    return 0;
}

/*
 * compute_fatigue() — Combines EMG and HR scores into a single level.
 *
 * total = emg_score + hr_score (max 4)
 *
 *   0 → NONE      (no indicators)
 *   1 → MILD      (weak single signal, e.g. elevated HR alone)
 *   2 → MODERATE  (EMG fatigue OR trend + moderate HR)
 *   3 → SEVERE    (EMG fatigue flag AND high HR — clamps at 3)
 *
 * We require the muscle to have been recently active (reps counted) before
 * elevating past MILD based solely on HR, to avoid false positives at
 * warm-up or during passive recovery.
 */
static fatigue_level_t compute_fatigue(const emg_metrics_t *emg, int32_t bpm)
{
    int emg_score = compute_emg_score(emg);
    int hr_score  = compute_hr_score(bpm);

    /* Suppress pure-HR fatigue until the user has done at least 2 reps. */
    if (emg_score == 0 && classifier_get_rep_count() < 2) {
        hr_score = 0;
    }

    int total = emg_score + hr_score;

    if (total >= 4) return FATIGUE_SEVERE;
    if (total == 3) return FATIGUE_SEVERE;
    if (total == 2) return FATIGUE_MODERATE;
    if (total == 1) return FATIGUE_MILD;
    return FATIGUE_NONE;
}

/* ══════════════════════════════════════════════════════════════════════════
 * Public API
 * ══════════════════════════════════════════════════════════════════════════ */

static classifier_result_t g_result = {
    .rep_count     = 0,
    .fatigue_level = FATIGUE_NONE,
    .new_rep       = false,
};

void classifier_init(void)
{
    rep_state         = REP_STATE_IDLE;
    state_ticks       = 0;
    rep_history_head  = 0;
    rep_history_count = 0;
    g_result.rep_count     = 0;
    g_result.fatigue_level = FATIGUE_NONE;
    g_result.new_rep       = false;
    printk("CLASSIFIER: Initialized (EMG+PPG, no IMU)\n");
}

classifier_result_t *classifier_update(void)
{
    /* Always clear new_rep at the start so it's only true for one tick. */
    g_result.new_rep = false;

    /* ── Step 1: Get fresh EMG metrics ─────────────────────────────── */
    int emg_err = EMG_compute_from_window();
    emg_metrics_t *emg = EMG_get_metrics();

    /* If the window isn't ready yet, don't advance the FSM. */
    if (emg_err != 0) {
        return &g_result;
    }

    /* ── Step 2: Rep detection FSM ──────────────────────────────────── */
    state_ticks++;

    switch (rep_state) {

    case REP_STATE_IDLE:
        if (emg->is_active) {
            rep_state   = REP_STATE_ACTIVE;
            state_ticks = 0;
            printk("CLASSIFIER: Rep started (EMG active)\n");
        }
        break;

    case REP_STATE_ACTIVE: {
        uint32_t active_ms = state_ticks * CLASSIFIER_PERIOD_MS;

        /* Timeout guard — if activation exceeds max, discard and reset */
        if (active_ms >= REP_MAX_ACTIVE_MS) {
            printk("CLASSIFIER: Activation timeout (%u ms) — discarded\n",
                active_ms);
            reset_rep_history();
            rep_state   = REP_STATE_IDLE;
            state_ticks = 0;
            break;
        }

        if (!emg->is_active) {
            if (active_ms >= REP_MIN_ACTIVE_MS) {
                g_result.rep_count++;
                g_result.new_rep = true;
                if (active_ms >= REP_MIN_ACTIVE_MS && active_ms <= REP_MAX_ACTIVE_MS) {
                    push_rep_snapshot(emg);
                }
                printk("CLASSIFIER: Rep #%d (active=%u ms, RMS=%d mV, ZCR=%d)\n",
                    g_result.rep_count, active_ms,
                    (int)emg->rms_mv, (int)emg->zcr);
                rep_state   = REP_STATE_COOLDOWN;
                state_ticks = 0;
            } else {
                printk("CLASSIFIER: Noise burst (%u ms) — discarded\n", active_ms);
                rep_state   = REP_STATE_IDLE;
                state_ticks = 0;
            }
        }
        break;
    }

    case REP_STATE_COOLDOWN: {
        uint32_t cooldown_ms = state_ticks * CLASSIFIER_PERIOD_MS;
        if (cooldown_ms >= REP_COOLDOWN_MS) {
            rep_state   = REP_STATE_IDLE;
            state_ticks = 0;
        }
        break;
    }
    }

    /* ── Step 3: Fatigue classification ─────────────────────────────── */
    int32_t bpm = ppg_get_bpm();
    g_result.fatigue_level = compute_fatigue(emg, bpm);

    printk("DBG EMG: rms=%d mav=%d zcr=%d active=%d dc=%d\n",
       (int)emg->rms_mv,
       (int)emg->mav_mv,
       (int)emg->zcr,
       (int)emg->is_active,
       (int)emg->dc_offset_mv);

    return &g_result;
}

uint16_t classifier_get_rep_count(void)
{
    return g_result.rep_count;
}

fatigue_level_t classifier_get_fatigue_level(void)
{
    return g_result.fatigue_level;
}