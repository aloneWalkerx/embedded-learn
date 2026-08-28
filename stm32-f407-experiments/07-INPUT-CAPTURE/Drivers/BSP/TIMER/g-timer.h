#ifndef __G_TIM_H
#define __G_TIM_H

#include "./SYSTEM/sys/sys.h"

/* 通用定时器定义 */

//定义定时器
#define GTIM_TIMX_CAP                       TIM5

//定时器中断
#define GTIM_TIMX_CAP_IRQn                  TIM5_IRQn

//定时器中断服务函数
#define GTIM_TIMX_CAP_IRQHandler            TIM5_IRQHandler

//使能定时器
#define GTIM_TIMX_CAP_CLK_ENABLE()          do { __HAL_RCC_TIM5_CLK_ENABLE(); } while (0)

//定义通道号号
#define GTIM_TIMX_CAP_CHY                   TIM_CHANNEL_1

//定义复用对应的引脚类型
#define GTIM_TIMX_CAP_CHY_GPIO_PORT         GPIOA

//定义复用的引脚号
#define GTIM_TIMX_CAP_CHY_GPIO_PIN          GPIO_PIN_0

//定义复用号
#define GTIM_TIMX_CAP_CHY_GPIO_AF           GPIO_AF2_TIM5

//使能复用对应的引脚时钟
#define GTIM_TIMX_CAP_CHY_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)


//初始化通用定时器输入捕获
void gtim_timx_cap_chy_init(uint16_t arr, uint16_t psc);


#endif
