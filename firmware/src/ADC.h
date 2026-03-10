#ifndef _ADC_H_
#define _ADC_H_

#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>

/* Define the configuration values directly */
#define SEQUENCE_SAMPLES 5
#define SEQUENCE_RESOLUTION 12

/* ADC node from the devicetree. */
#define ADC_NODE DT_ALIAS(adc0)

/* Auxiliary macro to obtain channel vref, if available. */
#define CHANNEL_VREF(node_id) DT_PROP_OR(node_id, zephyr_vref_mv, 0)

/* Data of ADC device specified in devicetree. */
static const struct device *adc = DEVICE_DT_GET(ADC_NODE);

/* Data array of ADC channels for the specified ADC - now only using channel 1 */
static const struct adc_channel_cfg channel_cfgs[] = {
    DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))};

/* Data array of ADC channel voltage references. */
static uint32_t vrefs_mv[] = {DT_FOREACH_CHILD_SEP(ADC_NODE, CHANNEL_VREF, (,))};

/* Get the number of channels defined on the DTS. */
#define CHANNEL_COUNT 1 

/* Initializes ADC for channels in overlay */
int ADC_init();

/* Reads the data from the channel and displays it */
void ADC_print();

#endif