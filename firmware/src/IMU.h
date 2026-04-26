#ifndef IMU_H
#define IMU_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

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
void IMU_sample(void);
imu_data_t *IMU_get_data(void);
void IMU_handler(struct k_timer *timer_id);

#ifdef __cplusplus
}
#endif

#endif /* IMU_H */