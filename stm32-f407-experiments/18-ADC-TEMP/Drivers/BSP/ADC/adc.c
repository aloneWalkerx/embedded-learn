#include "./BSP/ADC/adc.h"      

/* ====================================================================
   全局变量定义
   ==================================================================== */

// ★ ADC 句柄（用于内部温度传感器）
// 本实验使用 ADC1 的通道 16（内部温度传感器）
// 注意：内部温度传感器只能使用 ADC1，不能用 ADC2 或 ADC3
ADC_HandleTypeDef g_adc_temperature_handle = {0};

/* ====================================================================
   ① 初始化 ADC 采集内部温度传感器
   ==================================================================== */
/**
 * @brief   初始化ADC采集内部温度传感器
 * @param   无
 * @retval  无
 * @note    配置 ADC1 用于采集内部温度传感器（通道 16）
 *          温度传感器无需外部引脚，完全在芯片内部
 *          必须使能 ADC1 的内部温度传感器通道
 */
void adc_temperature_init(void)
{
    /* --- 步骤 1：配置 ADC 核心参数 --- */
    
    // ① 指定使用 ADC1（内部温度传感器只能使用 ADC1）
    g_adc_temperature_handle.Instance = ADC1;
    
    // ② 时钟分频：ADC 时钟 = PCLK2 / 4 = 84MHz / 4 = 21MHz
    g_adc_temperature_handle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    
    // ③ 分辨率：12 位（0~4095）
    g_adc_temperature_handle.Init.Resolution = ADC_RESOLUTION_12B;
    
    // ④ 数据对齐：右对齐
    g_adc_temperature_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    
    // ⑤ 扫描模式：禁用（单通道模式）
    // 只采集温度传感器一个通道，无需扫描
    g_adc_temperature_handle.Init.ScanConvMode = DISABLE;
    
    // ⑥ EOC 选择：序列转换结束
    g_adc_temperature_handle.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    
    // ⑦ 连续转换模式：禁用（单次转换，每次采样都需重新触发）
    // 温度变化缓慢，不需要连续转换
    g_adc_temperature_handle.Init.ContinuousConvMode = DISABLE;
    
    // ⑧ 规则组转换通道数量：1 个
    g_adc_temperature_handle.Init.NbrOfConversion = 1;
    
    // ⑨ 间断转换模式：禁用
    g_adc_temperature_handle.Init.DiscontinuousConvMode = DISABLE;
    
    // ⑩ 间断转换的通道数量：1 个
    g_adc_temperature_handle.Init.NbrOfDiscConversion = 1;
    
    // ? 外部触发源：软件触发
    g_adc_temperature_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    
    // ? 外部触发边沿：无（软件触发）
    g_adc_temperature_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    
    // ? DMA 连续请求：禁用（本实验不使用 DMA）
    g_adc_temperature_handle.Init.DMAContinuousRequests = DISABLE;
    
    // ? 调用 HAL 库函数完成 ADC 初始化
    // 此函数会自动调用 HAL_ADC_MspInit 配置时钟
    HAL_ADC_Init(&g_adc_temperature_handle);
    
    // ★ 注意：温度传感器的使能是在 HAL_ADC_MspInit 中通过
    //    __HAL_RCC_ADC1_CLK_ENABLE() 使能 ADC1 时钟，
    //    但温度传感器本身不需要额外的 GPIO 配置
}

/* ====================================================================
   ② HAL 库 ADC MSP 初始化函数（由 HAL_ADC_Init 自动调用）
   ==================================================================== */
/**
 * @brief   HAL库ADC初始化MSP函数
 * @param   hadc: ADC句柄
 * @retval  无
 * @note    此函数由 HAL_ADC_Init 自动调用
 *          作用是配置 ADC 的底层硬件资源：
 *          1. 使能 ADC1 时钟（温度传感器只需要 ADC1 时钟）
 *          2. ★ 内部温度传感器不需要配置 GPIO！
 *             因为它是内部信号，不是外部引脚
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    // ★ 判断是否是 ADC1
    // 内部温度传感器只能使用 ADC1
    if (hadc->Instance == ADC1)
    {
        // ★ 使能 ADC1 外设时钟
        // 内部温度传感器是 ADC1 的内部通道（通道 16）
        // 只需要使能 ADC1 时钟，不需要使能任何 GPIO 时钟
        __HAL_RCC_ADC1_CLK_ENABLE();
        
        // ★ 与外部 ADC 采样不同：
        // 这里不需要配置 GPIO 引脚！
        // 因为温度传感器在芯片内部，不需要外部引脚
        // 不需要 HAL_GPIO_Init(...)
    }
}

/* ====================================================================
   ③ 设置 ADC 通道（通用函数，可复用）
   ==================================================================== */
/**
 * @brief   设置ADC通道
 * @param   adc_handle:   ADC句柄指针
 * @param   channel:      ADC通道号（如 ADC_CHANNEL_TEMPSENSOR）
 * @param   rank:         规则组中的转换顺序（1~16）
 * @param   sampling_time: 采样时间（如 ADC_SAMPLETIME_480CYCLES）
 * @retval  无
 * @note    此函数通用，可用于任何 ADC 通道配置
 *          内部温度传感器建议使用较长的采样时间以获得更准确的结果
 */
void adc_channel_set(ADC_HandleTypeDef *adc_handle, uint32_t channel, uint32_t rank, uint32_t sampling_time)
{
    ADC_ChannelConfTypeDef adc_channel_conf_struct = {0};   // 通道配置结构体
    
    /* --- 配置 ADC 通道参数 --- */
    
    // ① 通道号（本实验使用 ADC_CHANNEL_TEMPSENSOR = 16）
    adc_channel_conf_struct.Channel = channel;
    
    // ② 规则组中的转换顺序（1 表示第一个转换）
    adc_channel_conf_struct.Rank = rank;
    
    // ③ 采样时间
    // ★ 温度传感器建议使用较长的采样时间（如 480 周期）
    // 因为内部温度传感器的输出阻抗较高，需要更多时间充电
    adc_channel_conf_struct.SamplingTime = sampling_time;
    
    // ④ 偏移量（本实验不使用）
    adc_channel_conf_struct.Offset = 0;
    
    // ★ 调用 HAL 库函数将配置写入硬件寄存器
    // 对于温度传感器，此函数会配置：
    // - ADC_SMPR1 寄存器（采样时间）
    // - ADC_SQR3 寄存器（规则序列）
    HAL_ADC_ConfigChannel(adc_handle, &adc_channel_conf_struct);
}

/* ====================================================================
   ④ 获取单次 ADC 转换结果（通用函数，可复用）
   ==================================================================== */
/**
 * @brief   获取ADC结果
 * @param   channel: ADC通道（如 ADC_CHANNEL_TEMPSENSOR）
 * @retval  12 位 ADC 转换结果（0~4095）
 * @note    内部流程：
 *          1. 配置通道（调用 adc_channel_set）
 *          2. 启动 ADC 转换（软件触发）
 *          3. 等待转换完成（轮询 EOC 标志）
 *          4. 读取并返回转换结果
 *          5. 此函数会阻塞 CPU，直到转换完成
 */
uint16_t adc_get_result(uint32_t channel)
{
    uint16_t result;   // 存储转换结果
    
    /* --- 步骤 1：配置 ADC 通道 --- */
    // 设置通道、排序位置（Rank=1）和采样时间（480 周期）
    // 温度传感器需要较长的采样时间以保证精度
    adc_channel_set(&g_adc_temperature_handle, channel, 1, ADC_SAMPLETIME_480CYCLES);
    
    /* --- 步骤 2：启动 ADC 转换（软件触发） --- */
    // HAL_ADC_Start 会：
    // 1. 使能 ADC 外设（ADEN 位 = 1）
    // 2. 发起软件触发（SWSTART 位 = 1）
    // 3. 开始转换
    HAL_ADC_Start(&g_adc_temperature_handle);
    
    /* --- 步骤 3：等待转换完成（阻塞轮询） --- */
    // HAL_ADC_PollForConversion 会：
    // 1. 轮询 EOC（转换结束）标志位
    // 2. 如果超时则返回超时错误，否则返回 HAL_OK
    // HAL_MAX_DELAY = 0xFFFFFFFF，表示一直等待直到转换完成
    HAL_ADC_PollForConversion(&g_adc_temperature_handle, HAL_MAX_DELAY);
    
    /* --- 步骤 4：读取并返回转换结果 --- */
    // HAL_ADC_GetValue 会：
    // 1. 读取 ADC_DR（数据寄存器）的值
    // 2. 返回 12 位结果（右对齐）
    result = HAL_ADC_GetValue(&g_adc_temperature_handle);
    
    return result;   // 返回 ADC 值（0~4095）
}

/* ====================================================================
   ⑤ 均值滤波获取 ADC 结果（通用函数，可复用）
   ==================================================================== */
/**
 * @brief   均值滤波获取ADC结果
 * @param   channel: ADC通道
 * @param   times:   均值滤波的采样次数（如 10 次）
 * @retval  滤波后的 ADC 结果（多次采样的平均值）
 * @note    原理：多次采样取平均值，消除随机噪声
 *          温度传感器输出存在一定噪声，均值滤波可以提高精度
 */
uint16_t adc_get_result_average(uint32_t channel, uint8_t times)
{
    uint32_t sum_result = 0;   // 累加器（32 位，防止溢出）
    uint8_t index;             // 循环计数器
    uint16_t result;           // 最终结果
    
    /* --- 步骤 1：多次采样并累加 --- */
    for (index = 0; index < times; index++)
    {
        // 调用 adc_get_result 获取单次转换结果
        // adc_get_result 内部会阻塞等待转换完成
        // 多次采样可以平滑随机噪声
        sum_result += adc_get_result(channel);
    }
    
    /* --- 步骤 2：计算平均值 --- */
    result = sum_result / times;   // 取平均
    
    return result;   // 返回均值滤波后的结果
}

/* ====================================================================
   ⑥ 获取内部温度传感器结果（核心函数）
   ==================================================================== */
/**
 * @brief   获取内部温度传感器结果
 * @param   无
 * @retval  内部温度传感器结果（扩大100倍）
 * @note    计算公式：
 *          temperature = (V_SENSE - V25) / Avg_Slope + 25
 *          
 *          简化公式（基于 STM32F407 典型值）：
 *          temperature = (voltage - 0.76) * 400 + 25
 *          其中：
 *          - 0.76V：25℃ 时的典型电压值（V25）
 *          - 400：温度斜率倒数（1 / 0.0025 = 400）
 *          - 25：25℃ 基准温度
 *          
 *          返回值示例：25.00℃ → 2500
 */
int16_t adc_get_temperature(void)
{
    uint16_t result;           // ADC 转换结果（0~4095）
    double voltage;            // 换算后的电压值（单位：V）
    double temperature;        // 计算后的温度值（单位：℃）
    int16_t temperature_x100;  // 扩大 100 倍后的温度值（用于 LCD 显示）
    
    /* --- 步骤 1：采集内部温度传感器的 ADC 值（10 次均值滤波） --- */
    // ADC_CHANNEL_TEMPSENSOR = 16（内部温度传感器通道）
    // 采样 10 次取平均，消除随机噪声
    result = adc_get_result_average(ADC_CHANNEL_TEMPSENSOR, 10);
    
    /* --- 步骤 2：将 ADC 值换算为电压值 --- */
    // ★ 电压计算公式：V = (ADC值 / 4095) × 3.3V
    // 例如：ADC = 2048 → 2048 / 4095 × 3.3 ≈ 1.65V
    // 使用 double 类型保证精度
    voltage = ((double)result * 3.3) / 4095;
    
    /* --- 步骤 3：将电压值换算为温度值 --- */
    // ★ 温度计算公式（基于 STM32F4 数据手册）：
    // temperature = (V_SENSE - V25) / Avg_Slope + 25
    // 
    // 简化公式（使用典型值）：
    // V25 = 0.76V（25℃ 时的电压）
    // Avg_Slope = 0.0025V/℃（温度每变化 1℃，电压变化 2.5mV）
    // 
    // 推导：
    // temperature = (voltage - 0.76) / 0.0025 + 25
    //             = (voltage - 0.76) × 400 + 25
    // 
    // 例如：电压 = 1.01V 时，
    // temperature = (1.01 - 0.76) × 400 + 25 = 100 + 25 = 125℃
    // 电压 = 0.51V 时，
    // temperature = (0.51 - 0.76) × 400 + 25 = -100 + 25 = -75℃
    temperature = (voltage - 0.76) * 400 + 25;
    
    /* --- 步骤 4：将温度值扩大 100 倍，转换为整数 --- */
    // ★ 为什么要扩大 100 倍？
    // 1. 避免浮点运算（浮点运算慢，占用资源）
    // 2. 便于在 LCD 上显示两位小数
    // 3. 便于传输和存储（整数比浮点更高效）
    // 
    // 例如：25.00℃ → 2500，25.50℃ → 2550
    temperature_x100 = (int16_t)(temperature * 100);
    
    /* --- 步骤 5：返回扩大 100 倍后的温度值 --- */
    return temperature_x100;
}

