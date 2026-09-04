#ifndef __DAC_H
#define __DAC_H

#include "./SYSTEM/sys/sys.h"

/* ====================================================================
   DAC 硬件配置宏（方便移植和修改）
   ==================================================================== */

// ★ ① DAC 外设选择：使用 DAC（STM32F4 只有一个 DAC 外设，包含两个独立通道）
// DAC 外设内部包含两个独立的 DAC 通道：
//   - DAC_CHANNEL_1 → PA4
//   - DAC_CHANNEL_2 → PA5
// 注意：DAC 与 ADC 不同，只有一个外设，不像 ADC1/2/3 有三个
#define DAC_DACX                        DAC

// ★ ② 使能 DAC 外设时钟的宏
// __HAL_RCC_DAC_CLK_ENABLE() 是 HAL 库提供的宏，用于开启 DAC 的时钟
// DAC 挂载在 APB1 总线上，时钟频率为 42MHz
// 如果不使能 DAC 时钟，访问 DAC 寄存器会出错（读回 0 或 HardFault）
// do { ... } while (0) 是标准的多语句宏写法，保证宏在任何地方都能安全展开
#define DAC_DACX_CLK_ENABLE()           do { __HAL_RCC_DAC_CLK_ENABLE(); } while (0)

// ★ ③ DAC 通道选择：DAC_CHANNEL_1
// 本实验使用 DAC 通道 1，对应输出引脚 PA4
// 可选值：DAC_CHANNEL_1 → PA4，DAC_CHANNEL_2 → PA5
// 两个通道可以独立工作，同时输出不同的电压
#define DAC_DACX_CHY                    DAC_CHANNEL_1

// ★ ④ 通道对应的 GPIO 端口
// PA4 属于 GPIOA 端口
// 如果使用 DAC_CHANNEL_2，这里应改为 GPIOA（PA5 也属于 GPIOA）
#define DAC_DACX_CHY_GPIO_PORT          GPIOA

// ★ ⑤ 通道对应的 GPIO 引脚编号
// DAC_CHANNEL_1 对应 PA4，引脚编号为 Pin 4
// 如果使用 DAC_CHANNEL_2，这里应改为 GPIO_PIN_5
#define DAC_DACX_CHY_GPIO_PIN           GPIO_PIN_4

// ★ ⑥ 使能 GPIO 端口的时钟宏
// PA4 属于 GPIOA，所以使能 GPIOA 时钟
// 注意：DAC 输出引脚必须配置为模拟模式（GPIO_MODE_ANALOG）
// 模拟模式下，引脚直接连接到 DAC 内部的模拟输出缓冲器
// 数字输入缓冲器和输出驱动器被禁用，避免引入噪声和干扰
#define DAC_DACX_CHY_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)

/* ====================================================================
   函数声明
   ==================================================================== */

/**
 * @brief   初始化 DAC
 * @note    包括：
 *          1. 使能 DAC 时钟（通过 DAC_DACX_CLK_ENABLE 宏）
 *          2. 配置 DAC 输出引脚 PA4 为模拟模式
 *          3. 配置 DAC 通道参数（触发方式、输出缓冲器）
 *          4. 启动 DAC 通道（开始输出）
 * @param   无
 * @retval  无
 */
void dac_init(void);

/**
 * @brief   设置 DAC 输出电压
 * @param   voltage: 目标电压值（单位：mV，范围：0~3300）
 *                   例如：voltage = 1650 → 输出 1.65V
 * @retval  无
 * @note    内部计算公式：
 *          value = (voltage × 4095) / 3300
 *          将毫伏值转换为 12 位数字量（0~4095）
 *          转换后的数字量被限制在 0~4095 范围内
 *          
 *          计算公式推导：
 *          value / 4095 = voltage / 3300
 *          value = (voltage × 4095) / 3300
 *          
 *          示例：
 *          voltage = 0    → value = 0     → 输出 0V
 *          voltage = 1650 → value = 2048  → 输出 1.65V
 *          voltage = 3300 → value = 4095  → 输出 3.3V
 */
void dac_set_voltage(uint16_t voltage);

/**
 * @brief   设置 DAC 输出三角波
 * @param   max_value: 三角波峰值对应的数字量（0~4095）
 *                     建议值：2048（峰值 1.65V）或 4095（峰值 3.3V）
 * @param   interval:  每个采样点的时间间隔（单位：微秒 μs）
 *                     值越小，波形频率越高
 *                     例如：interval = 100 → 每个点间隔 100μs
 * @param   samples:   一个三角波周期包含的采样点个数
 *                     必须为偶数（代码会自动调整为偶数）
 *                     值越大，波形越平滑
 *                     例如：samples = 100 → 上升 50 点 + 下降 50 点
 * @param   number:    输出三角波的周期个数
 *                     例如：number = 5 → 输出 5 个完整的三角波
 * @retval  无
 * @note    三角波原理：
 *          1. 从 0 开始，每 interval 微秒增加一个步进值
 *          2. 达到峰值后，每 interval 微秒减少一个步进值
 *          3. 回到 0 后完成一个周期
 *          
 *          步进值计算：
 *          incval = max_value / (samples / 2)
 *          
 *          波形示意图：
 *          电压 ▲
 *               │    /\      /\      /\
 *               │   /  \    /  \    /  \
 *               │  /    \  /    \  /    \
 *               │ /      \/      \/      \
 *               │/                     时间为
 *               └─────────────────────────?
 *           
 *          适用场景：音频测试、电机控制、信号发生器
 *          改进方向：配合 DMA + 定时器可实现更高效的波形生成
 */
void dac_triangular_wave(uint16_t max_value, uint16_t interval, uint16_t samples, uint16_t number);

#endif

