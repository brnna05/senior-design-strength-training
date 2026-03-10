/*
 * PPG.h - MAX30102 PPG Sensor Driver Header
 * Senior Design - Strength Training
 */

#ifndef PPG_H
#define PPG_H

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>

/* ── Devicetree ─────────────────────────────────────────────────────────── */
#define DT_DRV_COMPAT maxim_max30102

/* ── I2C Address ────────────────────────────────────────────────────────── */
#define MAX30102_I2C_ADDR 0x57

/* ── Register Map ───────────────────────────────────────────────────────── */
#define MAX30102_REG_INT_STS1   0x00
#define MAX30102_REG_FIFO_DATA  0x07
#define MAX30102_REG_FIFO_CFG   0x08
#define MAX30102_REG_MODE_CFG   0x09
#define MAX30102_REG_SPO2_CFG   0x0A
#define MAX30102_REG_LED1_PA    0x0C
#define MAX30102_REG_LED2_PA    0x0D
#define MAX30102_REG_PART_ID    0xFF

/* ── Constants ──────────────────────────────────────────────────────────── */
#define MAX30102_PART_ID            0x15
#define MAX30102_BYTES_PER_CHANNEL  3
#define MAX30102_MAX_NUM_CHANNELS   2
#define MAX30102_FIFO_DATA_MASK     0x3FFFF

/* ── Bit Masks / Shifts ─────────────────────────────────────────────────── */
#define MAX30102_MODE_CFG_RESET_MASK    (1 << 6)
#define MAX30102_FIFO_CFG_SMP_AVE_SHIFT 5
#define MAX30102_FIFO_CFG_ROLLOVER_MASK (1 << 4)
#define MAX30102_FIFO_CFG_FULL_SHIFT    0
#define MAX30102_SPO2_ADC_RGE_SHIFT     5
#define MAX30102_SPO2_SR_SHIFT          2
#define MAX30102_SPO2_PW_SHIFT          0

/* ── Pulse Width ────────────────────────────────────────────────────────── */
#define MAX30102_PW_18BITS  3

/* ── Heart Rate ─────────────────────────────────────────────────────────── */
#define HR_SAMPLE_RATE      200
#define HR_BUFFER_SIZE      200
#define HR_MIN_VALID_IR     5000

/* ── LED Channel Enum ───────────────────────────────────────────────────── */
enum max30102_led_channel {
    MAX30102_LED_CHANNEL_RED = 0,
    MAX30102_LED_CHANNEL_IR  = 1,
};

/* ── Config Struct ──────────────────────────────────────────────────────── */
struct max30102_config {
    const struct i2c_dt_spec i2c;
    uint8_t fifo;
    uint8_t mode;
    uint8_t spo2;
    uint8_t led_pa[MAX30102_MAX_NUM_CHANNELS];
};

/* ── Data Struct ────────────────────────────────────────────────────────── */
struct max30102_data {
    uint32_t raw[MAX30102_MAX_NUM_CHANNELS];
    uint32_t ir_buffer[HR_BUFFER_SIZE];
    uint8_t  ir_buf_idx;
    bool     ir_buf_full;
    int32_t  bpm;
};

/* ── Application API ────────────────────────────────────────────────────── */
int ppg_init(void);
void ppg_read(void);

#endif /* PPG_H */