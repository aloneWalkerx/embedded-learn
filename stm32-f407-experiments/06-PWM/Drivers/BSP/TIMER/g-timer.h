#ifndef  __G_TIMER_H
#define  __G_TIMER_H

#include "./SYSTEM/sys/sys.h"

//1，通用定时器定义

//使用TIM3定时器
#define GTIM_TIMX_INT                      TIM3

//通用定时器中断
#define GTIM_TIMX_INT_IRQn                 TIM3_IRQn

//通用定时中断服务函数
#define GTIM_TIMX_INT_IRQHandler           TIM3_IRQHandler

//使能通用定时器时钟
#define GTIM_TIMX_INT_CLK_ENABLE()          do { __HAL_RCC_TIM3_CLK_ENABLE();} while (0)


//2，设置PWM通道以及复用引脚

#define GTIM_TIMX_PWM                        TIM3

//使能PWM对应的通用定时器时钟
#define GTIM_TIMX_PWM_CLK_ENABLE()           do { __HAL_RCC_TIM3_CLK_ENABLE();} while (0)

//设置PWM对应的通用定时器通道
#define GTIM_TIMX_PWM_CHX                    TIM_CHANNEL_1

//设置PWM对应的引脚类型
#define GTIM_TIMX_PWM_CHX_GPIO_PORT          GPIOB

//设置PWM对应的引脚的引脚号
#define GTIM_TIMX_PWM_CHX_GPIO_PIN           GPIO_PIN_4

//设置PWM对应的引脚的复用号
#define GTIM_TIMX_PWM_CHX_GPIO_AF            GPIO_AF2_TIM3

//使能PWM对应通用定时器的引脚类型
#define GTIM_TIMX_PWM_CHX_GPIO_CLK_ENABLE()  do {__HAL_RCC_GPIOB_CLK_ENABLE();} while (0)


//3，函数声明

//初始化通用定时器中断
void gtim_timx_int_init(uint16_t arr, uint16_t psc);

//初始化通用定时器PWM
void gtim_timx_pwm_chy_init(uint16_t arr, uint16_t psc);

#endif 



