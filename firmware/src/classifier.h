#ifndef CLASSIFIER_H
#define CLASSIFIER_H
/*
 * classifier.h — Rep Detection & Fatigue Classification
 * Senior Design - Strength Training
 *
 * Uses EMG (primary) + PPG heart rate (secondary).
 * IMU is intentionally excluded (hardware unavailable).
 *
 * Rep detection:
 *   A 3-state FSM tracks EMG activation bursts:
 *     IDLE → ACTIVE (EMG goes active)
 *     ACTIVE → COOLDOWN (EMG goes quiet AND burst was long enough → rep counted)
 *     ACTIVE → IDLE (burst too short → noise, discarded)
 *     COOLDOWN → IDLE (after REP_COOLDOWN_MS rest)
 *
 * Fatigue classification (0–3):
 *   Score = emg_score (0–2) + hr_score (0–2), clamped to 0–3.
 *
 *   emg_score:
 *     2 → fatigue_flag set (active + ZCR below low threshold)
 *     1 → ZCR trend has dropped ≥25% over last 3 reps (median-freq shift)
 *     0 → no fatigue indicators
 *
 *   hr_score:
 *     2 → HR ≥ 150 BPM
 *     1 → HR ≥ 120 BPM
 *     0 → HR < 120 or no finger
 *
 *   total → FATIGUE_NONE(0), MILD(1), MODERATE(2), SEVERE(3)
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ── Fatigue level enum ──────────────────────────────────────────────── */
typedef enum {
    FATIGUE_NONE     = 0,  /* No indicators present                     */
    FATIGUE_MILD     = 1,  /* One weak signal (e.g. high HR alone)       */
    FATIGUE_MODERATE = 2,  /* EMG fatigue flag OR EMG trend + elevated HR*/
    FATIGUE_SEVERE   = 3,  /* EMG fatigue flag AND high HR               */
} fatigue_level_t;

/* ── Classifier output snapshot ─────────────────────────────────────── */
typedef struct {
    uint16_t        rep_count;      /* total reps counted this session   */
    fatigue_level_t fatigue_level;  /* current fatigue (0–3)             */
    bool            new_rep;        /* true for exactly one tick after rep*/
} classifier_result_t;

/* ── Public API ──────────────────────────────────────────────────────── */

/**
 * classifier_init() - Reset all state. Call once before sampling begins.
 */
void classifier_init(void);

/**
 * classifier_update() - Process latest EMG & PPG data.
 *
 * Must be called at a fixed CLASSIFIER_PERIOD_MS rate (default 100 ms).
 * Internally calls EMG_compute_from_window() and ppg_get_bpm().
 *
 * Returns a pointer to the internal result struct (never NULL).
 * new_rep is only true for a single call immediately after a rep is
 * counted; the caller must act on it before the next call.
 */
classifier_result_t *classifier_update(void);

/** Accessors — safe to call from any context after classifier_init(). */
uint16_t        classifier_get_rep_count(void);
fatigue_level_t classifier_get_fatigue_level(void);

#ifdef __cplusplus
}
#endif

#endif // CLASSIFIER_H