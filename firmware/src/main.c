#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/gpio.h>
#include "PPG.h"
#include "LSM6DS3TR.h"
#include "ADC.h"

#define PRINT_PERIOD_US  100 // 0.1 ms = 100 us
#define PRINT_PERIOD     K_USEC(PRINT_PERIOD_US)

static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
    BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
    BT_GAP_ADV_FAST_INT_MIN_2,
    BT_GAP_ADV_FAST_INT_MAX_2,
    NULL);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE,
            CONFIG_BT_DEVICE_NAME,
            sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

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

/* ── UUIDs ── keep these in sync with the RN app */
#define REP_SVC_UUID \
    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE( \
        0x12340001, 0x0000, 0x0000, 0x0000, 0x000000000001))

#define REP_CHAR_UUID \
    BT_UUID_DECLARE_128(BT_UUID_128_ENCODE( \
        0x12340001, 0x0000, 0x0000, 0x0000, 0x000000000002))

/* ── GATT service ── */
BT_GATT_SERVICE_DEFINE(rep_svc,
    BT_GATT_PRIMARY_SERVICE(REP_SVC_UUID),
    BT_GATT_CHARACTERISTIC(REP_CHAR_UUID,
        BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_NONE,
        NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* Send a single 0x01 byte to notify a rep */
static void notify_rep(void)
{
    static uint8_t rep_val = 0;
    rep_val++;
    /* attr index 1 = the characteristic value attribute */
    bt_gatt_notify(NULL, &rep_svc.attrs[1], &rep_val, sizeof(rep_val));
    printk("Notified rep = %d\n", rep_val);
}

/* ── Button 1 (gpio1, pin 9) ── */
static const struct gpio_dt_spec btn1 =
    GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);   /* sw1 = button1 in DTS */

static struct gpio_callback btn1_cb_data;

static void btn1_pressed(const struct device *dev,
                         struct gpio_callback *cb, uint32_t pins)
{
    notify_rep();
}

static int button_init(void)
{
    if (!gpio_is_ready_dt(&btn1)) {
        printk("Button GPIO not ready\n");
        return -ENODEV;
    }
    gpio_pin_configure_dt(&btn1, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&btn1, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&btn1_cb_data, btn1_pressed, BIT(btn1.pin));
    gpio_add_callback(btn1.port, &btn1_cb_data);
    printk("Button 1 ready\n");
    return 0;
}

int main(void)
{
    printk("starting firmware...\n");
	printk("ts_us,emg_mv,ax_mg,ay_mg,az_mg,gx_mdps,gy_mdps,gz_mdps,ir\n");

    /* BLE init */
    if (bt_enable(NULL)) {
        printk("BT enable failed\n");
        return -1;
    }

    if (bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0)) {
        printk("BT advertising failed\n");
        return -1;
    }

    printk("BLE advertising as \"%s\"\n", CONFIG_BT_DEVICE_NAME);
    
    button_init();

    // /* PPG init */
    // if (ppg_init(200)) {
    //     printk("PPG_Init failed.\n");
    //     return -1;
    // }
	// ir_val = ppg_get_data();

    // /* IMU init */
    // if (IMU_init(104)) {
    //     printk("IMU_Init failed.\n");
    //     return -1;
    // }
	// imu_data = IMU_get_data();


    // /* ADC init */
    // if (ADC_init()) {
    //     printk("ADC_Init failed.\n");
    //     return -1;
    // }
	// emg_raw = EMG_get_raw();

	// // k_work_init(&metrics_work, metrics_work_handler);
	// // k_timer_init(&metrics_timer, metrics_timer_handler, NULL);
	// // k_timer_start(&metrics_timer, K_MSEC(METRICS_PERIOD_MS), K_MSEC(METRICS_PERIOD_MS));


	// k_work_init(&print_work, print_work_handler);
    // k_timer_init(&print_timer, print_timer_handler, NULL);
    // k_timer_start(&print_timer, PRINT_PERIOD, PRINT_PERIOD);
 
    // printk("print timer started: every %d us\n", PRINT_PERIOD_US);

    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}