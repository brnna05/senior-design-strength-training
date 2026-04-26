#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/gpio.h>
#include "PPG.hpp"
#include "IMU.h"
#include "ADC.h"
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

/* ── Message types — keep in sync with RN app ── */
#define MSG_REP     0x01
#define MSG_BPM     0x02
#define MSG_FATIGUE 0x03

/* How often to send a BPM notification (ms).
 * The PPG buffer fills every 1 s at 200 Hz, so 1 s is the finest resolution. */
#define BPM_NOTIFY_PERIOD_MS 1000

static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
    BT_LE_ADV_OPT_CONNECTABLE,
    BT_GAP_ADV_FAST_INT_MIN_2,
    BT_GAP_ADV_FAST_INT_MAX_2,
    NULL);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE,
            CONFIG_BT_DEVICE_NAME,
            sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL,
        BT_UUID_128_ENCODE(0x12340001, 0x0000, 0x0000, 0x0000, 0x000000000001)),
};

/* ── UUIDs — keep in sync with RN app ── */
static const struct bt_uuid_128 rep_svc_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x12340001, 0x0000, 0x0000, 0x0000, 0x000000000001));

static const struct bt_uuid_128 rep_char_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x12340001, 0x0000, 0x0000, 0x0000, 0x000000000002));

/* ── GATT service ── */
BT_GATT_SERVICE_DEFINE(rep_svc,
    BT_GATT_PRIMARY_SERVICE(&rep_svc_uuid),
    BT_GATT_CHARACTERISTIC(&rep_char_uuid.uuid,
        BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_NONE,
        NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* ── Generic 2-byte notify helper [msgType, value] ── */
static void notify(uint8_t msg_type, uint8_t value)
{
    uint8_t buf[2] = { msg_type, value };
    bt_gatt_notify(NULL, &rep_svc.attrs[2], buf, sizeof(buf));
    printk("Notified type=0x%02x value=%d\n", msg_type, value);
}

/* ── Connection callback — send BPM=0 to initialise the RN app ── */
static void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed (err %u)\n", err);
        return;
    }
    printk("Connected — sending initial BPM=0\n");
    notify(MSG_BPM, 0);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason %u) — restarting advertising\n", reason);
    bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
}

static struct bt_conn_cb conn_callbacks = {
    .connected    = on_connected,
    .disconnected = on_disconnected,
};

/* ── BPM notify work + timer ────────────────────────────────────────────── */
static struct k_work bpm_work;
static struct k_timer bpm_timer;

static void bpm_work_handler(struct k_work *work)
{
    notify(MSG_FATIGUE, 0); // placeholder until we have real fatigue data
    int32_t bpm = ppg_get_bpm();

    if (bpm < 0) {
        /* Buffer not yet full or no finger — skip this tick */
        return;
    }

    /* BPM is validated to 40–180 by the PPG driver, safe to cast to uint8_t */
    notify(MSG_BPM, (uint8_t)bpm);
}

static void bpm_timer_handler(struct k_timer *t)
{
    k_work_submit(&bpm_work);
}

static const struct gpio_dt_spec ble_led = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), ble_led_gpios);
static const struct gpio_dt_spec imu_led = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), imu_led_gpios);
static const struct gpio_dt_spec ppg_led = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), ppg_led_gpios);
static const struct gpio_dt_spec emg_led = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), emg_led_gpios);

int main(void)
{
    if (!gpio_is_ready_dt(&ble_led)) {
        return -1;
    }
    if (!gpio_is_ready_dt(&imu_led)) {
        return -1;
    }
    if (!gpio_is_ready_dt(&ppg_led)) {
        return -1;
    }
    if (!gpio_is_ready_dt(&emg_led)) {
        return -1;
    }

    gpio_pin_configure_dt(&ble_led, GPIO_OUTPUT_INACTIVE); 
    gpio_pin_configure_dt(&imu_led, GPIO_OUTPUT_INACTIVE); 
    gpio_pin_configure_dt(&ppg_led, GPIO_OUTPUT_INACTIVE); 
    gpio_pin_configure_dt(&emg_led, GPIO_OUTPUT_INACTIVE); 

    bt_conn_cb_register(&conn_callbacks);

    if (bt_enable(NULL)) {
        printk("BT enable failed\n");
        return -1;
    }

    if (bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd))) {
        printk("BT advertising failed\n");
        return -1;
    }

    gpio_pin_set_dt(&ble_led, 1);
    printk("BLE advertising as \"%s\"\n", CONFIG_BT_DEVICE_NAME);

    /* BPM notify timer — fires every BPM_NOTIFY_PERIOD_MS */
    k_work_init(&bpm_work, bpm_work_handler);
    k_timer_init(&bpm_timer, bpm_timer_handler, NULL);
    k_timer_start(&bpm_timer,
                  K_MSEC(BPM_NOTIFY_PERIOD_MS),
                  K_MSEC(BPM_NOTIFY_PERIOD_MS));

    /* PPG — starts sampling at 200 Hz internally */
    if (ppg_init(HR_SAMPLE_RATE)) {
        printk("PPG init failed\n");
        return -1;
    }
    gpio_pin_set_dt(&ppg_led, 1);

    /* ADC/EMG — starts sampling at 4 kHz internally */
    if (ADC_init()) {
        printk("ADC init failed\n");
        return -1;
    }
    gpio_pin_set_dt(&emg_led, 1);

    /* IMU — starts sampling at 104 Hz internally */
    // if (IMU_init(104)) {
    //     printk("IMU_Init failed.\n");
    //     return -1;
    // }
    static const struct device *imu_dev = DEVICE_DT_GET(IMU_NODE);
    if (!device_is_ready(imu_dev)) {
        printk("IMU: Zephyr sensor device %s not ready\n", imu_dev->name);
        printk("IMU: Check your devicetree configuration\n");
        return -ENODEV;
    }
    gpio_pin_set_dt(&imu_led, 1);

    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}