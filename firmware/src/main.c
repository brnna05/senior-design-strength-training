/*
 * Copyright (c) 2024 Centro de Inovacao EDGE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <inttypes.h>

#include "ADC.h"


int main(void)
{
    if(ADC_init()){
        printk("ADC_Init failed. Trying again.");
    } // retry until ADC_Init succeeds
    
    printk("timestamp (ms), ADC_reading (mV)\n");
    while (1) {
        ADC_print();
        k_msleep(1000);
    }

    return 0;
}