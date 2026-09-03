
#ifndef __ADC_H
#define __ADC_H

#include "./SYSTEM/sys/sys.h"

//初始化ADC采集内部温度传感器
void adc_temperature_init(void);

//获取内部温度传感器结果
int16_t adc_get_temperature(void);

#endif


