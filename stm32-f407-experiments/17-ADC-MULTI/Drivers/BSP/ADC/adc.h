#ifndef __ADC_H
#define __ADC_H

#include "./SYSTEM/sys/sys.h"

/* ====================================================================
   多通道 ADC + DMA 硬件配置宏（支持双通道交替扫描）
   ==================================================================== */

// ★ ① ADC 外设选择：使用 ADC1
// 为什么用 ADC1？因为 ADC1 支持扫描模式，可以按顺序转换多个通道
// ADC1 的 DMA 请求映射到 DMA2_Stream0_Channel0（见 DMA 映射表）
#define ADC_NCH_DMA_ADCX                        ADC1

// ★ ② 使能 ADC1 外设时钟的宏
#define ADC_NCH_DMA_ADCX_CLK_ENABLE()           do { __HAL_RCC_ADC1_CLK_ENABLE(); } while (0)

// ★ ③ ADC 通道数量：2 个（双通道扫描模式）
// 在扫描模式下，ADC 会按顺序转换通道 A（PA5）和通道 B（PA6）
// 扫描顺序由 Rank 决定（Rank 1 = 通道 A，Rank 2 = 通道 B）
#define ADC_NCH_DMA_ADCX_CH_NUM                 2

// ★ ④ 通道 A（第 1 通道）：ADC_CHANNEL_5 → PA5
// PA5 是 ADC1 的通道 5，通常也用作 SPI1_SCK
// 本实验用 PA5 采集第 1 路模拟信号
#define ADC_NCH_DMA_ADCX_CHA                    ADC_CHANNEL_5

// ★ ⑤ 通道 A 对应的 GPIO 端口（PA5 属于 GPIOA）
#define ADC_NCH_DMA_ADCX_CHA_GPIO_PORT          GPIOA

// ★ ⑥ 通道 A 对应的 GPIO 引脚编号（Pin 5）
#define ADC_NCH_DMA_ADCX_CHA_GPIO_PIN           GPIO_PIN_5

// ★ ⑦ 使能 GPIOA 端口的时钟宏（PA5 和 PA6 共用 GPIOA）
#define ADC_NCH_DMA_ADCX_CHA_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)

// ★ ⑧ 通道 B（第 2 通道）：ADC_CHANNEL_6 → PA6
// PA6 是 ADC1 的通道 6，通常也用作 SPI1_MISO
// 本实验用 PA6 采集第 2 路模拟信号
#define ADC_NCH_DMA_ADCX_CHB                    ADC_CHANNEL_6

// ★ ⑨ 通道 B 对应的 GPIO 端口（PA6 属于 GPIOA）
#define ADC_NCH_DMA_ADCX_CHB_GPIO_PORT          GPIOA

// ★ ⑩ 通道 B 对应的 GPIO 引脚编号（Pin 6）
#define ADC_NCH_DMA_ADCX_CHB_GPIO_PIN           GPIO_PIN_6

// ★ ? 使能 GPIOA 端口的时钟宏（PA5 和 PA6 共用 GPIOA）
#define ADC_NCH_DMA_ADCX_CHB_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)

// ★ ? DMA 数据流选择：DMA2_Stream0
// 查 STM32F4 DMA 映射表可知，ADC1 的 DMA 请求映射到 DMA2_Stream0
// 数据流（Stream）之间可以并行工作，互不干扰
// 注意：必须使用 DMA2，不能用 DMA1
#define ADC_NCH_DMA_ADCX_DMASX                  DMA2_Stream0

// ★ ? 使能 DMA2 外设时钟的宏
// DMA1 和 DMA2 是独立的两个外设，都有自己独立的时钟开关
#define ADC_NCH_DMA_ADCX_DMA_CLK_ENABLE()       do { __HAL_RCC_DMA2_CLK_ENABLE(); } while (0)

// ★ ? DMA 通道选择：DMA_CHANNEL_0
// ADC1 的 DMA 请求固定映射到 DMA2_Stream0_Channel0
// 通道选择必须与数据流匹配，否则 DMA 无法响应外设请求
#define ADC_NCH_DMA_ADCX_DMASX_CHY              DMA_CHANNEL_0

// ★ ? DMA 中断号：DMA2_Stream0_IRQn
// 当 DMA 传输完成、半传输完成或发生错误时，触发此中断
#define ADC_NCH_DMA_ADCX_DMASX_IRQn             DMA2_Stream0_IRQn

// ★ ? DMA 中断服务函数名称
// 此函数由硬件中断自动调用，名称需与中断向量表一致
// 在 stm32f4xx_it.c 中实现，调用 HAL_DMA_IRQHandler
#define ADC_NCH_DMA_ADCX_DMASX_IRQHandler       DMA2_Stream0_IRQHandler

/* ====================================================================
   函数声明
   ==================================================================== */

/**
 * @brief   初始化多通道 ADC DMA 读取
 * @param   memory_base: DMA 目标内存的基地址（缓冲区数组的首地址）
 * @note    配置流程：
 *          1. 使能 ADC1 和 DMA2 时钟
 *          2. 配置 PA5、PA6 为模拟输入模式
 *          3. 配置 ADC1 为扫描模式 + 连续转换模式
 *          4. 配置 DMA2_Stream0 为循环模式，外设→内存，16 位
 *          5. 关联 DMA 和 ADC
 *          6. 扫描顺序：通道 A（PA5）→ 通道 B（PA6）
 * @retval  无
 */
void adc_nch_dma_init(uint32_t memory_base);

/**
 * @brief   开启多通道 ADC DMA 读取
 * @param   length: DMA 缓冲区长度（要采集的数据对数量）
 * @note    启动后：
 *          1. ADC1 开始连续扫描转换（PA5 → PA6 → PA5 → PA6 ...）
 *          2. DMA 自动将数据搬运到缓冲区
 *          3. 缓冲区数据排列：buf[0]=PA5, buf[1]=PA6, buf[2]=PA5, buf[3]=PA6 ...
 *          4. 采集完 length 个数据后，触发 DMA 中断
 * @retval  无
 */
void adc_nch_dma_enable(uint32_t length);

#endif    // 结束 __ADC_H 的条件编译

