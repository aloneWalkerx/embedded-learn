#ifndef __ADC_H
#define __ADC_H

#include "./SYSTEM/sys/sys.h"      

/* ====================================================================
   ADC + DMA 硬件配置宏（方便移植和修改）
   ==================================================================== */

// ★ ① ADC 外设选择：使用 ADC3
// 为什么用 ADC3？因为 ADC1 和 ADC2 可能被其他外设（如定时器、USART）占用
// ADC3 相对空闲，适合做独立的 DMA 连续采样
// 可选值：ADC1、ADC2、ADC3
#define ADC_DMA_ADCX                            ADC3

// ★ ② 使能 ADC3 外设时钟的宏
// __HAL_RCC_ADC3_CLK_ENABLE() 是 HAL 库提供的宏，用于开启 ADC3 的时钟
// 如果不使能 ADC 时钟，访问 ADC 寄存器会出错
#define ADC_DMA_ADCX_CLK_ENABLE();              do { __HAL_RCC_ADC3_CLK_ENABLE(); } while (0)

// ★ ③ 选择 ADC 通道（本实验使用 Channel 1）
// ADC3 的 Channel 1 对应 GPIO 引脚 PA1
// 其他选项：ADC_CHANNEL_0(PA0)、ADC_CHANNEL_1(PA1)、ADC_CHANNEL_2(PA2) 等
#define ADC_DMA_ADCX_CHY                        ADC_CHANNEL_1

// ★ ④ 通道对应的 GPIO 端口（PA1 属于 GPIOA 端口）
#define ADC_DMA_ADCX_CHY_GPIO_PORT              GPIOA

// ★ ⑤ 通道对应的 GPIO 引脚编号（Pin 1）
#define ADC_DMA_ADCX_CHY_GPIO_PIN               GPIO_PIN_1

// ★ ⑥ 使能 GPIOA 端口的时钟宏
// 注意：ADC 引脚必须配置为模拟输入模式，不需要配置复用功能（AF）
#define ADC_DMA_ADCX_CHY_GPIO_CLK_ENABLE()      do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)

// ★ ⑦ DMA 数据流选择：DMA2_Stream1
// 查 STM32F4 数据手册可知，ADC3 的 DMA 请求映射到 DMA2_Stream1
// DMA2 支持内存到内存和外设到内存，功能比 DMA1 更强
// 数据流（Stream）之间可以并行工作，互不干扰
#define ADC_DMA_ADCX_DMASX                      DMA2_Stream1

// ★ ⑧ 使能 DMA2 外设时钟的宏
// DMA1 和 DMA2 是独立的两个外设，都有自己独立的时钟开关
#define ADC_DMA_ADCX_DMA_CLK_ENABLE()           do { __HAL_RCC_DMA2_CLK_ENABLE(); } while (0)

// ★ ⑨ DMA 通道选择：DMA_CHANNEL_2
// ADC3 的 DMA 请求固定映射到 DMA2_Stream1_Channel2
// 通道选择必须与数据流匹配，否则 DMA 无法响应外设请求
// 这是硬件决定的，不能随意更改
#define ADC_DMA_ADCX_DMASX_CHY                  DMA_CHANNEL_2

// ★ ⑩ DMA 中断号：DMA2_Stream1_IRQn
// 当 DMA 传输完成、半传输完成或发生错误时，触发此中断
// 中断向量名称必须与 startup_stm32f407xx.s 中的向量表一致
#define ADC_DMA_ADCX_DMASX_IRQn                 DMA2_Stream1_IRQn

// ★ ? DMA 中断服务函数名称
// 此函数由硬件中断自动调用，名称需与中断向量表一致
// 在 stm32f4xx_it.c 中实现，调用 HAL_DMA_IRQHandler
#define ADC_DMA_ADCX_DMASX_IRQHandler           DMA2_Stream1_IRQHandler



/* ====================================================================
   函数声明
   ==================================================================== */

/**
 * @brief   初始化 ADC + DMA
 * @param   memory_base: DMA 目标内存的基地址（缓冲区数组的首地址）
 * @note    配置流程：
 *          1. 使能 ADC3 和 DMA2 时钟
 *          2. 配置 PA1 为模拟输入模式
 *          3. 配置 ADC3 为连续转换模式
 *          4. 配置 DMA2_Stream1 为循环模式，外设→内存
 *          5. 关联 DMA 和 ADC
 * @retval  无
 */
void adc_dma_init(uint32_t memory_base);

/**
 * @brief   开启 ADC DMA 读取
 * @param   length: DMA 缓冲区长度（要采集的数据个数）
 * @note    启动后：
 *          1. ADC3 开始连续转换
 *          2. DMA2 自动将 ADC_DR 的数据搬运到内存缓冲区
 *          3. 循环模式下，DMA 填满缓冲区后会从头继续填
 *          4. CPU 完全释放，可以执行其他任务
 * @retval  无
 */
void adc_dma_enable(uint32_t length);

#endif    // 结束 __ADC_H 的条件编译

