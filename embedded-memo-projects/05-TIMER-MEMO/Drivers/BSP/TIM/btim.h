#ifndef __BTIM_H
#define __BTIM_H

#include "./SYSTEM/sys/sys.h"                       // 包含 sys.h，里面引入了 stm32f4xx.h、HAL 库等

/* ---------- 基本定时器硬件相关宏 ---------- */
#define BTIM_TIMX                   TIM6            // 使用 TIM6（基本定时器）
#define BTIM_TIMX_IRQn              TIM6_DAC_IRQn   // TIM6 的中断号（与 DAC 共用）
#define BTIM_TIMX_IRQHandler        TIM6_DAC_IRQHandler // 中断服务函数名（启动文件里定义）
#define BTIM_TIMX_CLK_ENABLE()      do{__HAL_RCC_TIM6_CLK_ENABLE();}while(0) // 使能 TIM6 时钟

void btim_init(uint16_t arr, uint16_t psc);         // 初始化函数声明：arr 重装载值、psc 预分频

#endif
