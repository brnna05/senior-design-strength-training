#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "PPG.h"
#include "LSM6DS3TR.h"

#define ADC_ON 1
#define PPG_ON 0
#define IMU_ON 0
#define BLE_ON 0

#if BLE_ON
static const struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
    BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
    BT_GAP_ADV_FAST_INT_MIN_2,
    BT_GAP_ADV_FAST_INT_MAX_2,
    NULL);
#endif

int main(void)
{
    printk("Starting Senior Design Strength Training Firmware\n");
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

#if PPG_ON
    /* PPG init */
    if (ppg_init()) {
        printk("PPG_Init failed.\n");
        return -1;
    }
#endif

#if IMU_ON
    /* IMU init */
    if (IMU_init(104)) {
        printk("IMU_Init failed.\n");
        return -1;
    }
#endif

#if ADC_ON
    /* ADC init */
    if (ADC_init()) {
        printk("ADC_Init failed.\n");
        return -1;
    }
#endif

    while (1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}