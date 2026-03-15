#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "PPG.h"
#include "LSM6DS3TR.h"
#include "ADC.h"

#define BLE_ON 0

#define PRINT_PERIOD_US  100 // 0.1 ms = 100 us
#define PRINT_PERIOD     K_USEC(PRINT_PERIOD_US)

#if BLE_ON
static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
    BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
    BT_GAP_ADV_FAST_INT_MIN_2,
    BT_GAP_ADV_FAST_INT_MAX_2,
    NULL);
#endif

static struct sensor_value *ir_val;
static imu_data_t *imu_data;
static int32_t *emg_raw;

static struct k_work print_work;
static struct k_timer print_timer;

static void print_work_handler(struct k_work *work) {
	int64_t ts_us = k_ticks_to_us_floor64(k_uptime_ticks());


	printk("%lld,%d,%d,%d,%d,%d,%d,%d,%d\n",
        ts_us,
    	*emg_raw,
    	imu_data->accel_x, imu_data->accel_y, imu_data->accel_z,
    	imu_data->gyro_x, imu_data->gyro_y, imu_data->gyro_z,
    	ir_val->val1);
}

static void print_timer_handler(struct k_timer *timer_id)
{
    k_work_submit(&print_work);
}

#define METRICS_PERIOD_MS   100
static struct k_work  metrics_work;
static struct k_timer metrics_timer;

static void metrics_work_handler(struct k_work *work)
{
    if (EMG_compute_from_window() == 0) {
        int64_t ts = k_ticks_to_us_floor64(k_uptime_ticks());
        printk("%lld,emg_rms=%d,mav=%d,p2p=%d,zcr=%u,active=%d,fatigue=%d\n",
               ts,
               emg_data.rms_mv,
               emg_data.mav_mv,
               emg_data.peak_to_peak_mv,
               emg_data.zcr,
               (int)emg_data.is_active,
               (int)emg_data.fatigue_flag);
    }
}

static void metrics_timer_handler(struct k_timer *t)
{
    k_work_submit(&metrics_work);
}

int main(void)
{
    printk("starting firmware...\n");
	printk("ts_us,emg_mv,ax_mg,ay_mg,az_mg,gx_mdps,gy_mdps,gz_mdps,ir\n");

    /* BLE init */
#if BLE_ON
    if (bt_enable(NULL)) {
        printk("BT enable failed\n");
        return -1;
    }

    if (bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0)) {
        printk("BT advertising failed\n");
        return -1;
    }

    printk("BLE advertising as \"%s\"\n", CONFIG_BT_DEVICE_NAME);
#endif

    /* PPG init */
    if (ppg_init(200)) {
        printk("PPG_Init failed.\n");
        return -1;
    }
	ir_val = ppg_get_data();

    /* IMU init */
    if (IMU_init(104)) {
        printk("IMU_Init failed.\n");
        return -1;
    }
	imu_data = IMU_get_data();


    /* ADC init */
    if (ADC_init()) {
        printk("ADC_Init failed.\n");
        return -1;
    }
	emg_raw = EMG_get_raw();

	// k_work_init(&metrics_work, metrics_work_handler);
	// k_timer_init(&metrics_timer, metrics_timer_handler, NULL);
	// k_timer_start(&metrics_timer, K_MSEC(METRICS_PERIOD_MS), K_MSEC(METRICS_PERIOD_MS));


	k_work_init(&print_work, print_work_handler);
    k_timer_init(&print_timer, print_timer_handler, NULL);
    k_timer_start(&print_timer, PRINT_PERIOD, PRINT_PERIOD);
 
    printk("print timer started: every %d us\n", PRINT_PERIOD_US);

    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}