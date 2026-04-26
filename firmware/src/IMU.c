/*
 * IMU.c - LSM6DSO IMU Driver
 * Senior Design - Strength Training
 *
 * - Timer-based periodic sampling (accel + gyro)
 * - Hardware interrupt on INT1 for significant motion detection
 */

#include "IMU.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

/* Devices */
static const struct device *imu_dev = DEVICE_DT_GET(IMU_NODE);

static const struct gpio_dt_spec int1_gpio =
    GPIO_DT_SPEC_GET(DT_NODELABEL(lsm6dso), irq_gpios);

/* Work + Timer */
static struct k_work imu_work;
static struct gpio_callback gpio_cb;

K_TIMER_DEFINE(imu_timer, IMU_handler, NULL);

/* Timer Callback (interrupt context) */
void IMU_handler(struct k_timer *timer_id)
{
    k_work_submit(&imu_work);
}

/* Work Handler (work queue thread) */
static void imu_work_handler(struct k_work *work)
{
    IMU_sample();
}

/* Significant Motion Interrupt Handler */
static void int1_handler(const struct device *dev,
                         struct gpio_callback *cb, uint32_t pins)
{
    printk("IMU: Significant motion detected\n");
}

// Debug function to probe I2C device
int i2c_probe_device(const struct device *i2c_dev, uint16_t addr, const char *name)
{
    uint8_t test_byte = 0;
    
    printk("I2C: Probing %s at 0x%02x...\n", name, addr);
    
    // Try to read one byte from WHO_AM_I (0x0F for LSM6DSO)
    int ret = i2c_reg_read_byte(i2c_dev, addr, 0x0F, &test_byte);
    
    if (ret < 0) {
        printk("I2C: Failed to probe %s: %d\n", name, ret);
        return ret;
    }
    
    printk("I2C: %s responded to probe, WHO_AM_I=0x%02x\n", name, test_byte);
    return 0;
}

// Function to read WHO_AM_I register
int i2c_read_whoami(const struct device *i2c_dev, uint16_t addr, const char *name, 
                     uint8_t whoami_reg, uint8_t expected)
{
    uint8_t whoami = 0;
    int ret = i2c_reg_read_byte(i2c_dev, addr, whoami_reg, &whoami);
    
    if (ret < 0) {
        printk("I2C: Failed to read WHO_AM_I from %s: %d\n", name, ret);
        return ret;
    }
    
    printk("I2C: %s WHO_AM_I = 0x%02x (expected 0x%02x)\n", name, whoami, expected);
    
    if (whoami != expected) {
        printk("I2C: %s WHO_AM_I mismatch!\n", name);
        return -EIO;
    }
    
    return 0;
}

// Function to dump all relevant registers
void i2c_dump_registers(const struct device *i2c_dev, uint16_t addr, const char *name,
                         uint8_t start_reg, uint8_t num_regs)
{
    uint8_t regs[32] = {0};
    int ret;
    
    printk("I2C: %s Register dump (0x%02x+%d):\n", name, start_reg, num_regs);
    
    for (int i = 0; i < num_regs; i++) {
        ret = i2c_reg_read_byte(i2c_dev, addr, start_reg + i, &regs[i]);
        if (ret < 0) {
            printk("  Failed to read register 0x%02x: %d\n", start_reg + i, ret);
            return;
        }
        
        if (i % 8 == 0) {
            printk("  %02x: ", start_reg + i);
        }
        printk("%02x ", regs[i]);
        if ((i + 1) % 8 == 0 || i == num_regs - 1) {
            printk("\n");
        }
    }
}

// I2C bus scanner
void i2c_scan_bus(const struct device *i2c_dev)
{
    uint8_t data;
    int ret;
    
    printk("I2C: Scanning bus for devices...\n");
    printk("     ");
    for (uint8_t i = 0; i < 16; i++) {
        printk("%02x ", i);
    }
    printk("\n");
    
    for (uint8_t addr_base = 0x00; addr_base <= 0x70; addr_base += 0x10) {
        printk("0x%02x: ", addr_base);
        for (uint8_t offset = 0; offset < 0x10; offset++) {
            uint8_t addr = addr_base + offset;
            if (addr < 0x08 || addr > 0x77) {
                printk("   ");
                continue;
            }
            
            // Try to read any register
            ret = i2c_reg_read_byte(i2c_dev, addr, 0x0F, &data);
            
            if (ret >= 0) {
                printk("%02x ", addr);
                
                // Identify known devices
                if (addr == 0x6a || addr == 0x6b) {
                    uint8_t whoami = 0;
                    i2c_reg_read_byte(i2c_dev, addr, 0x0F, &whoami);
                    if (whoami == 0x6C) {
                        printk("[LSM6DSO] ");
                    }
                } else if (addr == 0x57) {
                    printk("[MAX30102] ");
                }
            } else {
                printk("-- ");
            }
        }
        printk("\n");
    }
    printk("I2C: Scan complete\n");
}

imu_data_t imu_data;

int IMU_init(int sample_rate_hz)
{
    int ret;
    const struct device *i2c_bus = DEVICE_DT_GET(DT_NODELABEL(i2c21));
    
    if (!device_is_ready(i2c_bus)) {
        printk("I2C: Bus i2c21 not ready!\n");
        return -ENODEV;
    }
    printk("I2C: Bus i2c21 is ready\n");
    
    // Scan the bus first
    i2c_scan_bus(i2c_bus);
    
    // Try both possible addresses for LSM6DSO
    uint16_t imu_addr = 0x6a;
    bool found = false;
    
    // Try address 0x6a
    ret = i2c_probe_device(i2c_bus, 0x6a, "LSM6DSO");
    if (ret == 0) {
        // Read WHO_AM_I to confirm
        ret = i2c_read_whoami(i2c_bus, 0x6a, "LSM6DSO", 0x0F, 0x6C);
        if (ret == 0) {
            found = true;
            imu_addr = 0x6a;
        }
    }
    
    // Try address 0x6b if 0x6a didn't work
    if (!found) {
        ret = i2c_probe_device(i2c_bus, 0x6b, "LSM6DSO");
        if (ret == 0) {
            ret = i2c_read_whoami(i2c_bus, 0x6b, "LSM6DSO", 0x0F, 0x6C);
            if (ret == 0) {
                found = true;
                imu_addr = 0x6b;
            }
        }
    }
    
    if (!found) {
        printk("IMU: No LSM6DSO device found on I2C bus!\n");
        printk("IMU: Please check:\n");
        printk("  - Power supply to IMU (VDD and VDDIO)\n");
        printk("  - I2C pull-up resistors (2.2k-10k ohms)\n");
        printk("  - CS pin tied to VDDIO (I2C mode)\n");
        printk("  - SA0 pin for address (GND=0x6a, VDDIO=0x6b)\n");
        return -ENODEV;
    }
    
    // Dump control registers before configuration
    printk("IMU: Dumping registers before init\n");
    i2c_dump_registers(i2c_bus, imu_addr, "LSM6DSO", 0x10, 8);  // CTRL1~CTL8
    i2c_dump_registers(i2c_bus, imu_addr, "LSM6DSO", 0x01, 1);  // WHO_AM_I
    
    // Try to read device ready status directly
    uint8_t status = 0;
    ret = i2c_reg_read_byte(i2c_bus, imu_addr, 0x1E, &status); // STATUS_REG
    if (ret == 0) {
        printk("IMU: STATUS_REG = 0x%02x\n", status);
    } else {
        printk("IMU: Failed to read STATUS_REG: %d\n", ret);
    }
    
    // Check if Zephyr sensor device is ready
    if (!device_is_ready(imu_dev)) {
        printk("IMU: Zephyr sensor device %s not ready\n", imu_dev->name);
        printk("IMU: Check your devicetree configuration\n");
        return -ENODEV;
    }
    
    // Reset the device to known state
    printk("IMU: Resetting device...\n");
    ret = i2c_reg_write_byte(i2c_bus, imu_addr, 0x12, 0x01); // CTRL3_C - SW_RESET
    if (ret < 0) {
        printk("IMU: Failed to reset device: %d\n", ret);
        return ret;
    }
    k_msleep(100); // Wait for reset to complete
    
    // Configure ODR and power modes
    // CTRL1_XL (0x10) - Accelerometer ODR and full scale
    printk("IMU: Configuring accelerometer...\n");
    ret = i2c_reg_write_byte(i2c_bus, imu_addr, 0x10, 0x60); // 416 Hz, 4g
    if (ret < 0) {
        printk("IMU: Failed to configure accelerometer: %d\n", ret);
        return ret;
    }
    
    // CTRL2_G (0x11) - Gyroscope ODR and full scale
    printk("IMU: Configuring gyroscope...\n");
    ret = i2c_reg_write_byte(i2c_bus, imu_addr, 0x11, 0x60); // 416 Hz, 1000 dps
    if (ret < 0) {
        printk("IMU: Failed to configure gyroscope: %d\n", ret);
        return ret;
    }
    
    // Verify configuration was written
    uint8_t verify_reg = 0;
    ret = i2c_reg_read_byte(i2c_bus, imu_addr, 0x10, &verify_reg);
    if (ret == 0) {
        printk("IMU: CTRL1_XL verification: 0x%02x (expected 0x60)\n", verify_reg);
    }
    
    ret = i2c_reg_read_byte(i2c_bus, imu_addr, 0x11, &verify_reg);
    if (ret == 0) {
        printk("IMU: CTRL2_G verification: 0x%02x (expected 0x60)\n", verify_reg);
    }
    
    imu_data.accel_x = 0;
    imu_data.accel_y = 0;
    imu_data.accel_z = 0;
    imu_data.gyro_x  = 0;
    imu_data.gyro_y  = 0;
    imu_data.gyro_z  = 0;
    
    uint32_t imu_period_us = 1000000U / (uint32_t)sample_rate_hz;
    k_work_init(&imu_work, imu_work_handler);
    k_timer_start(&imu_timer, K_USEC(imu_period_us), K_USEC(imu_period_us));
    
    printk("IMU: LSM6DSO initialized successfully at %d Hz\n", sample_rate_hz);
    
    // Final register dump to confirm
    printk("IMU: Final register state\n");
    i2c_dump_registers(i2c_bus, imu_addr, "LSM6DSO", 0x10, 8);
    
    return 0;
}

void IMU_sample(void)
{
    int err;
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
    imu_data.accel_x = (int32_t)(accel[0].val1 * 1000 + accel[0].val2 / 1000);
    imu_data.accel_y = (int32_t)(accel[1].val1 * 1000 + accel[1].val2 / 1000);
    imu_data.accel_z = (int32_t)(accel[2].val1 * 1000 + accel[2].val2 / 1000);

    /* Convert rad/s → mdps */
    imu_data.gyro_x = (int32_t)((gyro[0].val1 * 1000000LL + gyro[0].val2) * 1000LL / 17453LL);
    imu_data.gyro_y = (int32_t)((gyro[1].val1 * 1000000LL + gyro[1].val2) * 1000LL / 17453LL);
    imu_data.gyro_z = (int32_t)((gyro[2].val1 * 1000000LL + gyro[2].val2) * 1000LL / 17453LL);
}

imu_data_t *IMU_get_data(void) { return &imu_data; }