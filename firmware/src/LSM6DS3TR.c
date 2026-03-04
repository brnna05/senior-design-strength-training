/*
 * IMU.c - LSM6DSL IMU Driver
 * Senior Design - Strength Training
 *
 * - Timer-based periodic sampling (accel + gyro)
 * - Hardware interrupt on INT1 for significant motion detection
 */

#include "LSM6DS3TR.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* ── Devices ────────────────────────────────────────────────────────────── */
static const struct device *imu_dev = DEVICE_DT_GET(IMU_NODE);

static const struct gpio_dt_spec int1_gpio =
    GPIO_DT_SPEC_GET(DT_NODELABEL(lsm6dsl), irq_gpios);

/* ── Work + Timer ───────────────────────────────────────────────────────── */
static struct k_work imu_work;
static struct gpio_callback gpio_cb;

K_TIMER_DEFINE(imu_timer, IMU_handler, NULL);

/* ── Timer Callback (interrupt context) ────────────────────────────────── */
void IMU_handler(struct k_timer *timer_id)
{
    k_work_submit(&imu_work);
}

/* ── Work Handler (work queue thread) ──────────────────────────────────── */
static void imu_work_handler(struct k_work *work)
{
    IMU_print();
}

/* ── Significant Motion Interrupt Handler ───────────────────────────────── */
static void int1_handler(const struct device *dev,
                         struct gpio_callback *cb, uint32_t pins)
{
    printk("IMU: Significant motion detected\n");
    /* Motion detected - can be used to trigger other actions if needed */
}

/* ── Init ───────────────────────────────────────────────────────────────── */
int IMU_init(int sample_rate_hz)
{
    /* Check IMU device is ready */
    if (!device_is_ready(imu_dev)) {
        printk("IMU: device %s not ready\n", imu_dev->name);
        return -ENODEV;
    }

    /* Set accelerometer ODR */
    struct sensor_value odr = {
        .val1 = sample_rate_hz,
        .val2 = 0,
    };

    if (sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ,
                        SENSOR_ATTR_SAMPLING_FREQUENCY, &odr) < 0) {
        printk("IMU: could not set accel ODR to %d Hz\n", sample_rate_hz);
        return -EIO;
    }

    if (sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ,
                        SENSOR_ATTR_SAMPLING_FREQUENCY, &odr) < 0) {
        printk("IMU: could not set gyro ODR to %d Hz\n", sample_rate_hz);
        return -EIO;
    }

    /* ── Configure INT1 GPIO for significant motion interrupt ── */
    if (!device_is_ready(int1_gpio.port)) {
        printk("IMU: INT1 GPIO not ready\n");
        return -ENODEV;
    }

    if (gpio_pin_configure_dt(&int1_gpio, GPIO_INPUT) < 0) {
        printk("IMU: failed to configure INT1 pin\n");
        return -EIO;
    }

    if (gpio_pin_interrupt_configure_dt(&int1_gpio,
                                         GPIO_INT_EDGE_TO_ACTIVE) < 0) {
        printk("IMU: failed to configure INT1 interrupt\n");
        return -EIO;
    }

    gpio_init_callback(&gpio_cb, int1_handler, BIT(int1_gpio.pin));
    gpio_add_callback(int1_gpio.port, &gpio_cb);

    printk("IMU: INT1 significant motion interrupt configured on P1.05\n");

    /* ── Start periodic sampling timer ── */
    uint32_t imu_period_us = 1000000U / (uint32_t)sample_rate_hz;
    k_work_init(&imu_work, imu_work_handler);
    k_timer_start(&imu_timer, K_USEC(imu_period_us), K_USEC(imu_period_us));

    printk("IMU: initialized at %d Hz\n", sample_rate_hz);
    return 0;
}

/* ── Print ──────────────────────────────────────────────────────────────── */
void IMU_print(void)
{
    int err;
    int64_t timestamp_ms = k_uptime_get();
    struct sensor_value accel[3], gyro[3];

    err = sensor_sample_fetch(imu_dev);
    if (err < 0) {
        printk("IMU: sample fetch failed (%d)\n", err);
        return;
    }

    err = sensor_channel_get(imu_dev, SENSOR_CHAN_ACCEL_XYZ, accel);
    if (err < 0) {
        printk("IMU: accel channel get failed (%d)\n", err);
        return;
    }

    err = sensor_channel_get(imu_dev, SENSOR_CHAN_GYRO_XYZ, gyro);
    if (err < 0) {
        printk("IMU: gyro channel get failed (%d)\n", err);
        return;
    }

    /* Convert m/s² → mg */
    int32_t ax_mg = (int32_t)(accel[0].val1 * 1000 + accel[0].val2 / 1000);
    int32_t ay_mg = (int32_t)(accel[1].val1 * 1000 + accel[1].val2 / 1000);
    int32_t az_mg = (int32_t)(accel[2].val1 * 1000 + accel[2].val2 / 1000);

    /* Convert rad/s → mdps */
    int32_t gx_mdps = (int32_t)((gyro[0].val1 * 1000000LL + gyro[0].val2) * 1000LL / 17453LL);
    int32_t gy_mdps = (int32_t)((gyro[1].val1 * 1000000LL + gyro[1].val2) * 1000LL / 17453LL);
    int32_t gz_mdps = (int32_t)((gyro[2].val1 * 1000000LL + gyro[2].val2) * 1000LL / 17453LL);

    printk("%lld, %d, %d, %d, %d, %d, %d\n",
           timestamp_ms,
           ax_mg, ay_mg, az_mg,
           gx_mdps, gy_mdps, gz_mdps);
}