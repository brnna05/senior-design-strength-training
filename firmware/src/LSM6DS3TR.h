/*
 * LSM6DS3TR.h - Compatible with LSM6DSL IMU Driver
 * Senior Design - Strength Training
 */

#ifndef LSM6DS3TR_H
#define LSM6DS3TR_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

/* ── Devicetree ─────────────────────────────────────────────────────────── */
#define IMU_NODE DT_ALIAS(imu0)

/* ── IMU Data Structure ─────────────────────────────────────────────────── */
typedef struct {
    /* Accelerometer (mg) */
    int32_t accel_x;
    int32_t accel_y;
    int32_t accel_z;

    /* Gyroscope (mdps) */
    int32_t gyro_x;
    int32_t gyro_y;
    int32_t gyro_z;
} imu_data_t;

/* ── Function Declarations ──────────────────────────────────────────────── */

/* Initializes IMU, configures INT1 significant motion interrupt,
 * and starts periodic sampling timer at sample_rate_hz */
int IMU_init(int sample_rate_hz);

/* Samples and stores IMU data */
void IMU_sample(void);

/* Returns pointer to latest IMU data struct */
imu_data_t *IMU_get_data(void);

/* Timer callback — do not call directly */
void IMU_handler(struct k_timer *timer_id);

#endif /* LSM6DS3TR_H */