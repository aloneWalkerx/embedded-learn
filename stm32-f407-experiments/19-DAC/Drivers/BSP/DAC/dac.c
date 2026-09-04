#include "./BSP/DAC/dac.h"
#include "./SYSTEM/delay/delay.h"

/* ====================================================================
   全局变量定义
   ==================================================================== */

// ★ DAC 句柄（HAL 库用来管理 DAC 外设的核心结构体）
// 包含了 DAC 的所有配置参数、状态信息和回调函数指针
// 本实验使用 DAC 通道 1（PA4）
DAC_HandleTypeDef g_dac_handle = {0};

/* ====================================================================
   ① DAC 初始化函数
   ==================================================================== */
/**
 * @brief   初始化DAC
 * @param   无
 * @retval  无
 * @note    配置 DAC 并启动输出：
 *          1. 设置 DAC 实例
 *          2. 调用 HAL_DAC_Init（自动调用 HAL_DAC_MspInit 配置 GPIO 和时钟）
 *          3. 启动 DAC 通道（开始输出）
 */
void dac_init(void)
{
    /* --- 步骤 1：配置 DAC 实例 --- */
    // ① 指定使用哪个 DAC 外设（由 dac.h 中的宏 DAC_DACX 决定）
    // DAC_DACX 定义为 DAC（STM32F4 只有一个 DAC 外设）
    g_dac_handle.Instance = DAC_DACX;
    
    /* --- 步骤 2：初始化 DAC 硬件 --- */
    // ★ HAL_DAC_Init 会：
    // 1. 检查参数有效性
    // 2. 自动调用 HAL_DAC_MspInit（用户实现，配置 GPIO 和时钟）
    // 3. 初始化 DAC 硬件寄存器
    HAL_DAC_Init(&g_dac_handle);
    
    /* --- 步骤 3：启动 DAC 通道 --- */
    // ★ HAL_DAC_Start 使能 DAC 通道，开始输出
    // 参数 1：DAC 句柄
    // 参数 2：DAC 通道（由 dac.h 中的宏 DAC_DACX_CHY 决定，本实验为 DAC_CHANNEL_1）
    // 启动后，DAC 会持续输出当前设置的值（初始为 0V）
    HAL_DAC_Start(&g_dac_handle, DAC_DACX_CHY);
}

/* ====================================================================
   ② HAL 库 DAC MSP 初始化函数（由 HAL_DAC_Init 自动调用）
   ==================================================================== */
/**
 * @brief   HAL库DAC初始化MSP函数
 * @param   hdac: DAC句柄
 * @retval  无
 * @note    此函数由 HAL_DAC_Init 自动调用，不需要用户手动调用
 *          作用是配置 DAC 的底层硬件资源：
 *          1. 使能 DAC 时钟
 *          2. 使能 GPIO 时钟
 *          3. 配置 DAC 输出引脚为模拟模式
 *          4. 配置 DAC 通道参数（触发方式、输出缓冲器）
 */
void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac)
{
    GPIO_InitTypeDef gpio_init_struct = {0};        // GPIO 配置结构体
    DAC_ChannelConfTypeDef dac_channel_conf_struct = {0};  // DAC 通道配置结构体
    
    // ★ 判断是否是我们要初始化的 DAC 外设
    if (hdac->Instance == DAC_DACX)
    {
        /* --- 步骤 1：使能外设时钟 --- */
        
        // ① 使能 DAC 外设时钟（由 dac.h 中的宏定义）
        // ★ 必须使能 DAC 时钟，否则无法访问 DAC 寄存器！
        DAC_DACX_CLK_ENABLE();
        
        // ② 使能 DAC 引脚的 GPIO 时钟
        // 本实验使用 PA4（属于 GPIOA），所以使能 GPIOA 时钟
        DAC_DACX_CHY_GPIO_CLK_ENABLE();
        
        /* --- 步骤 2：配置 DAC 输出引脚 --- */
        
        // ③ 配置引脚为模拟模式
        // ★ GPIO_MODE_ANALOG 是 DAC 输出的关键配置！
        // 模拟模式下，引脚直接连接到 DAC 内部的模拟输出缓冲器
        // 数字输入缓冲器和输出驱动器被禁用，避免引入噪声
        gpio_init_struct.Pin = DAC_DACX_CHY_GPIO_PIN;   // 引脚号（PA4）
        gpio_init_struct.Mode = GPIO_MODE_ANALOG;       // ★ 模拟模式
        gpio_init_struct.Pull = GPIO_NOPULL;            // 无上下拉（模拟模式不需要）
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;  // 速度（模拟模式无影响）
        HAL_GPIO_Init(DAC_DACX_CHY_GPIO_PORT, &gpio_init_struct);
        
        /* --- 步骤 3：配置 DAC 通道参数 --- */
        
        // ④ ★ 配置 DAC 触发方式
        // DAC_TRIGGER_NONE：软件触发（写入数据寄存器立即更新输出）
        // 其他选项：定时器触发（如 TIMx_TRGO）、外部中断触发等
        // 本实验使用软件触发，调用 HAL_DAC_SetValue 时立即生效
        dac_channel_conf_struct.DAC_Trigger = DAC_TRIGGER_NONE;
        
        // ⑤ ★ 配置 DAC 输出缓冲器
        // DAC_OUTPUTBUFFER_DISABLE：禁用输出缓冲器
        //   - 输出阻抗高（约 15kΩ），带负载能力弱
        //   - 精度略高（适合高精度、低负载场景）
        // DAC_OUTPUTBUFFER_ENABLE：使能输出缓冲器（推荐）
        //   - 输出阻抗低（约 10kΩ），驱动能力强
        //   - 精度略降（适合普通应用）
        // 本实验禁用缓冲器（为了更高精度），但实际项目通常使能
        dac_channel_conf_struct.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
        
        // ⑥ 调用 HAL 库函数将通道配置写入硬件寄存器
        // 此函数会配置 DAC_CR 寄存器（使能、触发、缓冲器等）
        // 参数 1：DAC 句柄
        // 参数 2：通道配置结构体
        // 参数 3：DAC 通道（DAC_CHANNEL_1 或 DAC_CHANNEL_2）
        HAL_DAC_ConfigChannel(&g_dac_handle, &dac_channel_conf_struct, DAC_DACX_CHY);
    }
}

/* ====================================================================
   ③ 设置 DAC 输出电压（核心功能函数）
   ==================================================================== */
/**
 * @brief   设置DAC输出电压
 * @param   voltage: DAC输出电压（单位：mV，范围：0~3300）
 * @retval  无
 * @note    将毫伏值转换为 12 位数字量并写入 DAC 数据寄存器
 *          转换公式：value = (voltage × 4095) / 3300
 *          示例：voltage = 1650 → value = 2048 → 输出 1.65V
 */
void dac_set_voltage(uint16_t voltage)
{
    uint16_t value;   // 12 位数字量（0~4095）
    
    /* --- 步骤 1：将电压值（mV）转换为 12 位数字量 --- */
    // ★ 电压转数字量公式：value = (voltage / 3300) × 4095
    // 先乘后除，避免浮点运算（整数运算更快）
    // 例如：voltage = 1650 → 1650 × 4095 / 3300 = 2048
    value = (voltage * 4095) / 3300;
    
    /* --- 步骤 2：确保数字量在 12 位范围内（0~4095） --- */
    // ★ 0xFFF = 4095，与 0xFFF 按位与可以清除高 4 位
    // 确保 value 不会超过 4095（防止输入电压超过 3300mV 导致溢出）
    value &= 0xFFF;   // 相当于 value = value % 4096
    
    /* --- 步骤 3：将数字量写入 DAC 数据寄存器 --- */
    // ★ HAL_DAC_SetValue 会立即更新 DAC 输出（因为触发方式为 DAC_TRIGGER_NONE）
    // 参数 1：DAC 句柄
    // 参数 2：DAC 通道（DAC_CHANNEL_1 或 DAC_CHANNEL_2）
    // 参数 3：数据对齐方式（DAC_ALIGN_12B_R = 12 位右对齐）
    // 参数 4：12 位数字量（0~4095）
    // 
    // ★ 写入后，PA4 引脚的电压会立即改变
    // 建立时间约 3~5μs（输出缓冲器禁用时稍长）
    HAL_DAC_SetValue(&g_dac_handle, DAC_DACX_CHY, DAC_ALIGN_12B_R, value);
}
 