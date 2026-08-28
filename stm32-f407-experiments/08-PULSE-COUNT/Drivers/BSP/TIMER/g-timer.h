#ifndef __G_TIMER_H
#define __G_TIMER_H

#include "./SYSTEM/sys/sys.h"

//对应定时器
#define GTIM_TIMX_CNT                       TIM2

//对应定时器中断
#define GTIM_TIMX_CNT_IRQn                  TIM2_IRQn

//对应定时器中断服务函数
#define GTIM_TIMX_CNT_IRQHandler            TIM2_IRQHandler

//使能对应定时器时钟
#define GTIM_TIMX_CNT_CLK_ENABLE()          do { __HAL_RCC_TIM2_CLK_ENABLE(); } while (0)

//对应定时器通道
#define GTIM_TIMX_CNT_CHY                   TIM_CHANNEL_1

//复用引脚类型
#define GTIM_TIMX_CNT_CHY_GPIO_PORT         GPIOA

//复用引脚号
#define GTIM_TIMX_CNT_CHY_GPIO_PIN          GPIO_PIN_0

//复用号
#define GTIM_TIMX_CNT_CHY_GPIO_AF           GPIO_AF1_TIM2

//使能复用引脚时钟
#define GTIM_TIMX_CNT_CHY_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)


//初始化通用定时器脉冲计数
void gtim_timx_cnt_chy_init(uint16_t psc);

//获取通用定时器脉冲计数值
uint32_t gtim_timx_cnt_chy_get_count(void);

//重启通用定时器脉冲计数
void gtim_timx_cnt_chy_restart(void);


#endif

