#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "ADC.h"

#define ADC_FREQUENCY 1000 /* TODO: change to ~4k for final */
#define ADC_PERIOD K_USEC(1000000 / ADC_FREQUENCY) 

/* Use 16-bit values for ADC readings */
uint16_t channel_reading[SEQUENCE_SAMPLES];

/* Options for the sequence sampling. */
const struct adc_sequence_options options = {
    .extra_samplings = SEQUENCE_SAMPLES - 1,
    .interval_us = 0,
};

/* Configure the sampling sequence to be made. */
struct adc_sequence sequence = {
    .buffer = channel_reading,
    .buffer_size = sizeof(channel_reading),
    .resolution = SEQUENCE_RESOLUTION,
    .options = &options,
    .channels = 0,  /* changed in init */
};

static struct k_work adc_work;

void ADC_handler(struct k_timer *timer_id) {
    k_work_submit(&adc_work);
}

void adc_work_handler(struct k_work *work) {
    ADC_print();  
}

K_TIMER_DEFINE(adc_timer, ADC_handler, NULL);

int ADC_init() {
    int err;
    
    if (!device_is_ready(adc)) {
        printk("ADC controller device %s not ready\n", adc->name);
        return -1;
    }

    /* Setup the channel */
    sequence.channels = BIT(channel_cfgs[0].channel_id);
    err = adc_channel_setup(adc, &channel_cfgs[0]);
    if (err < 0) {
        printk("Could not setup channel (%d)\n", err);
        return -1;
    }
    
    /* Get vref if needed */
    if ((vrefs_mv[0] == 0) && (channel_cfgs[0].reference == ADC_REF_INTERNAL)) {
        vrefs_mv[0] = adc_ref_internal(adc);
    }

    /* Initialize timer */
    k_work_init(&adc_work, adc_work_handler);
    k_timer_start(&adc_timer, ADC_PERIOD, ADC_PERIOD);
    
    return 0;
}

void ADC_print() {
    int err;
    int64_t timestamp_ms = k_uptime_get();
    
    printk("%lld,", timestamp_ms);

    err = adc_read(adc, &sequence);
    if (err < 0) {
        printk("Could not read (%d)\n", err);
        return;
    }

    int32_t val_mv = 0;
    int sum = 0;
        
    for (size_t i = 0U; i < SEQUENCE_SAMPLES; i++) {

        val_mv = channel_reading[i];
        err = adc_raw_to_millivolts(vrefs_mv[0], channel_cfgs[0].gain, SEQUENCE_RESOLUTION, &val_mv);
        /* conversion to mV may not be supported, skip if not */
	    if (!((err < 0) || vrefs_mv[0] == 0)) {
            sum += val_mv;
	    }
    }

    val_mv = sum / SEQUENCE_SAMPLES;
	

    printk(" %d\n", val_mv);
}