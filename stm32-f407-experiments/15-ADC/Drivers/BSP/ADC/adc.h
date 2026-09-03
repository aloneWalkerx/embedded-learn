#ifndef __ADC_H                    // 如果未定义 __ADC_H 宏，则编译以下代码
#define __ADC_H                    // 定义 __ADC_H 宏，防止头文件被重复包含

#include "./SYSTEM/sys/sys.h"      // 包含系统基础头文件（类型定义、位操作等）

/* ====================================================================
   ADC 硬件配置宏（方便移植和修改）
   ==================================================================== */

// ★ 选择 ADC 外设（本实验使用 ADC2）
// 可选值：ADC1、ADC2、ADC3
// 使用 ADC2 的原因：ADC1 可能与 USART3 等外设冲突，ADC2 更空闲
#define ADC_ADCX                                ADC2

// ★ 使能 ADC2 外设时钟的宏
// __HAL_RCC_ADC2_CLK_ENABLE() 是 HAL 库提供的宏，用于开启 ADC2 的时钟
// do { ... } while (0) 是标准的多语句宏写法，保证宏在任何地方都能安全展开
#define ADC_ADCX_CLK_ENABLE()                   do { __HAL_RCC_ADC2_CLK_ENABLE(); } while (0)

// ★ 选择 ADC 通道（本实验使用 Channel 1）
// ADC2 的 Channel 1 对应 GPIO 引脚 PA1
// 其他选项：ADC_CHANNEL_0(PA0)、ADC_CHANNEL_1(PA1)、ADC_CHANNEL_2(PA2) 等
#define ADC_ADCX_CHY                            ADC_CHANNEL_1

// ★ 通道对应的 GPIO 端口（PA1 属于 GPIOA 端口）
#define ADC_ADCX_CHY_GPIO_PORT                  GPIOA

// ★ 通道对应的 GPIO 引脚编号（Pin 1）
#define ADC_ADCX_CHY_GPIO_PIN                   GPIO_PIN_1

// ★ 使能 GPIOA 端口的时钟宏
// 注意：ADC 引脚必须配置为模拟输入模式，不需要配置复用功能（AF）
// 模拟输入模式下，引脚被直接连接到 ADC 内部的采样保持电路
#define ADC_ADCX_CHY_GPIO_CLK_ENABLE()          do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)

/* ====================================================================
   函数声明
   ==================================================================== */

/**
 * @brief   初始化 ADC
 * @note    包括：
 *          1. 使能 ADC 时钟
 *          2. 配置 ADC 参数（分辨率、转换模式、数据对齐等）
 *          3. 配置 ADC 通道（采样时间等）
 *          4. 校准 ADC（提高精度）
 * @param   无
 * @retval  无
 */
void adc_init(void);

/**
 * @brief   设置 ADC 通道（动态配置通道）
 * @param   adc_handle:    ADC 句柄指针
 * @param   channel:       通道号（如 ADC_CHANNEL_1）
 * @param   rank:          规则组中的转换顺序（1~16）
 * @param   sampling_time: 采样时间（如 ADC_SAMPLETIME_84CYCLES）
 * @retval  无
 * @note    此函数用于在运行时切换 ADC 通道，方便多通道采样
 *          如果不频繁切换通道，可以直接在 adc_init() 中配置
 */
void adc_channel_set(ADC_HandleTypeDef *adc_handle, uint32_t channel, uint32_t rank, uint32_t sampling_time);

/**
 * @brief   获取单次 ADC 转换结果（阻塞方式）
 * @param   channel: 通道号（如 ADC_CHANNEL_1）
 * @retval  12 位 ADC 转换结果（0~4095）
 * @note    函数内部流程：
 *          1. 启动 ADC 转换（软件触发）
 *          2. 等待转换完成（轮询 EOC 标志）
 *          3. 读取转换结果并返回
 */
uint16_t adc_get_result(uint32_t channel);

/**
 * @brief   多次采样取平均值（软件滤波）
 * @param   channel: 通道号
 * @param   times:   采样次数（如 10 次）
 * @retval  滤波后的 ADC 值（多次采样的平均值）
 * @note    优点：消除随机噪声，提高测量稳定性
 *          缺点：速度变慢（采样次数越多，耗时越长）
 *          典型应用：光敏传感器、温度传感器等易受干扰的场景
 */
uint16_t adc_get_result_average(uint32_t channel, uint8_t times);

#endif    // 结束 __ADC_H 的条件编译

