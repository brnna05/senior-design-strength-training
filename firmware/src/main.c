#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>

int main(void)
{
    int err = bt_enable(NULL);
    if (err) {
        printk("BLE init failed (%d)\n", err);
        return 0;
    }

    printk("Bluetooth ready\n");

    while (1) {
        k_sleep(K_SECONDS(1));
    }
}
