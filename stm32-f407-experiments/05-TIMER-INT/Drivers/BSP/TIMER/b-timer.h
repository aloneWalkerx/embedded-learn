#ifndef  __B_TIMER_H
#define  __B_TIMER_H

#include "./SYSTEM/sys/sys.h"

//1，基本定时器定义
//选择TIM6基本定时器
#define  BTIM_TIMX_INT                      TIM6

//定义TIM6定时器对应的中断                  
#define  BTIM_TIMX_INT_IRQn                 TIM6_DAC_IRQn

//定义中断服务函数
#define  BTIM_TIMX_INT_IRQHandler           TIM6_DAC_IRQHandler

//使能定时器
#define  BTIM_TIMX_INT_CLK_ENABLE()           do { __HAL_RCC_TIM6_CLK_ENABLE();} while (0)

//2，初始化基本定时器
void btim_timx_int_init(uint16_t arr , uint16_t psc);


#endif



