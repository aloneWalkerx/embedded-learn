#ifndef __RTC_H
#define __RTC_H

#include "./SYSTEM/sys/sys.h"

//读取后备寄存器
uint16_t rtc_read_bkr(uint32_t bkrx);

//写入后备寄存器
void rtc_write_bkr(uint32_t bkrx, uint16_t data);

//初始化RTC
uint8_t rtc_init(void);

//设置RTC时间信息
uint8_t rtc_set_time(uint8_t hour, uint8_t minute, uint8_t second, uint8_t ampm);

//设置RTC日期信息
uint8_t rtc_set_date(uint8_t year, uint8_t month, uint8_t date, uint8_t week);

//获取RTC时间信息
void rtc_get_time(uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *ampm);

//获取RTC日期信息
void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *week);

//年月日转星期
uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t date);

//设置RTC闹钟时间信息
void rtc_set_alarm(uint8_t week, uint8_t hour, uint8_t minute, uint8_t second);

//设置RTC周期性唤醒中断
void rtc_set_wakeup(uint8_t clock, uint8_t count);

// 获取RTC闹钟时间信息
void rtc_get_alarm(uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *week);

#endif