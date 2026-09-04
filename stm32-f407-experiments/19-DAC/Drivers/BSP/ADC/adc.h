#ifndef __ADC_H
#define __ADC_H

#include "./SYSTEM/sys/sys.h"

/* ====================================================================
   ADC 硬件配置宏（方便移植和修改）
   ==================================================================== */

// ★ ① ADC 外设选择：使用 ADC2
// 为什么用 ADC2？因为 ADC1 可能与 USART3 等外设冲突，ADC2 相对空闲
// 可选值：ADC1、ADC2、ADC3
// 注意：内部温度传感器只能用 ADC1，普通外部采集三个都可以用
#define ADC_ADCX                                ADC2

// ★ ② 使能 ADC2 外设时钟的宏
// __HAL_RCC_ADC2_CLK_ENABLE() 是 HAL 库提供的宏，用于开启 ADC2 的时钟
// 如果不使能 ADC 时钟，访问 ADC 寄存器会出错（HardFault 或读回 0）
// do { ... } while (0) 是标准的多语句宏写法，保证宏在任何地方都能安全展开
#define ADC_ADCX_CLK_ENABLE()                   do { __HAL_RCC_ADC2_CLK_ENABLE(); } while (0)

// ★ ③ ADC 通道选择：Channel 1
// ADC2 的 Channel 1 对应 GPIO 引脚 PA1
// 常用通道与引脚对应关系：
//   ADC_CHANNEL_0 → PA0   ADC_CHANNEL_1 → PA1
//   ADC_CHANNEL_2 → PA2   ADC_CHANNEL_3 → PA3
//   ADC_CHANNEL_4 → PA4   ADC_CHANNEL_5 → PA5
//   ...
// 修改此宏即可切换 ADC 采样引脚，无需改动 .c 文件
#define ADC_ADCX_CHY                            ADC_CHANNEL_1

// ★ ④ 通道对应的 GPIO 端口
// PA1 属于 GPIOA 端口
// 如果换到 PB0，这里应改为 GPIOB
#define ADC_ADCX_CHY_GPIO_PORT                  GPIOA

// ★ ⑤ 通道对应的 GPIO 引脚编号
// PA1 的引脚编号是 Pin 1
// 如果换到 PB0，这里应改为 GPIO_PIN_0
#define ADC_ADCX_CHY_GPIO_PIN                   GPIO_PIN_1

// ★ ⑥ 使能 GPIO 端口的时钟宏
// PA1 属于 GPIOA，所以使能 GPIOA 时钟
// 注意：ADC 引脚必须配置为模拟输入模式（GPIO_MODE_ANALOG）
// 模拟输入模式下，引脚直接连接到 ADC 内部的采样保持电路
// 数字输入缓冲器和输出驱动器被禁用，避免引入噪声
#define ADC_ADCX_CHY_GPIO_CLK_ENABLE()          do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)

/* ====================================================================
   函数声明
   ==================================================================== */

/**
 * @brief   初始化 ADC
 * @note    包括：
 *          1. 使能 ADC 时钟（通过 ADC_ADCX_CLK_ENABLE 宏）
 *          2. 配置 ADC 参数（分辨率、转换模式、数据对齐等）
 *          3. 自动调用 HAL_ADC_MspInit 配置 GPIO 引脚
 *          4. 不包含通道配置（通道在 adc_get_result 中动态配置）
 * @param   无
 * @retval  无
 */
void adc_init(void);

/**
 * @brief   设置 ADC 通道（动态配置通道）
 * @param   adc_handle:    ADC 句柄指针（通常传 &g_adc_handle）
 * @param   channel:       通道号（如 ADC_CHANNEL_1）
 * @param   rank:          规则组中的转换顺序（1~16，单通道填 1）
 * @param   sampling_time: 采样时间（如 ADC_SAMPLETIME_480CYCLES）
 * @retval  无
 * @note    此函数用于在运行时切换 ADC 通道，方便多通道采样
 *          本实验在 adc_get_result 中被调用，每次读取前重新配置通道
 *          如果不频繁切换通道，也可以直接在 adc_init 中配置
 */
void adc_channel_set(ADC_HandleTypeDef *adc_handle, uint32_t channel, uint32_t rank, uint32_t sampling_time);

/**
 * @brief   获取单次 ADC 转换结果（阻塞方式）
 * @param   channel: 通道号（如 ADC_CHANNEL_1）
 * @retval  12 位 ADC 转换结果（0~4095）
 * @note    函数内部流程：
 *          1. 调用 adc_channel_set 配置通道
 *          2. 调用 HAL_ADC_Start 启动转换（软件触发）
 *          3. 调用 HAL_ADC_PollForConversion 等待转换完成（轮询 EOC 标志）
 *          4. 调用 HAL_ADC_GetValue 读取结果并返回
 *          ★ 此函数会阻塞 CPU，直到转换完成（约 23.4μs）
 *          适合低频采集（如温度、电压读数），不适合高频采集（如音频）
 */
uint16_t adc_get_result(uint32_t channel);

/**
 * @brief   多次采样取平均值（软件滤波）
 * @param   channel: 通道号
 * @param   times:   采样次数（如 10 次）
 * @retval  滤波后的 ADC 值（多次采样的平均值）
 * @note    优点：消除随机噪声，提高测量稳定性
 *          缺点：速度变慢（采样次数越多，耗时越长）
 *          典型应用：光敏传感器、温度传感器、DAC 回读验证等易受干扰的场景
 *          
 *          本实验用于 DAC 电压回读，10 次采样取平均后显示更稳定
 *          如果 times=10，总耗时 ≈ 10 × 23.4μs = 234μs
 *          配合主循环 100ms 延时，CPU 占用率极低（< 0.3%）
 */
uint16_t adc_get_result_average(uint32_t channel, uint8_t times);

#endif
