#include "./BSP/ADC/adc.h"

/* ====================================================================
   全局变量定义
   ==================================================================== */

// ★ ADC 句柄（HAL 库用来管理 ADC 外设的核心结构体）
// 包含了 ADC 的所有配置参数、状态信息和回调函数指针
// 本实验使用 ADC2，通道 1（PA1）
ADC_HandleTypeDef g_adc_handle = {0};

/* ====================================================================
   ① ADC 初始化函数
   ==================================================================== */
/**
 * @brief   初始化ADC
 * @param   无
 * @retval  无
 * @note    配置 ADC 的核心参数，包括：
 *          - 时钟分频（ADC_CLOCK_SYNC_PCLK_DIV4 → 21MHz）
 *          - 分辨率（12 位）
 *          - 数据对齐（右对齐）
 *          - 转换模式（单次转换，非连续）
 *          - 触发方式（软件触发）
 *          - 关闭 DMA（本实验不使用）
 */
void adc_init(void)
{
    /* --- 步骤 1：配置 ADC 核心参数 --- */
    
    // ① 指定使用哪个 ADC 外设（由 adc.h 中的宏 ADC_ADCX 决定）
    // 本实验使用 ADC2
    g_adc_handle.Instance = ADC_ADCX;
    
    // ② 时钟分频：ADC 时钟 = PCLK2 / 4 = 84MHz / 4 = 21MHz
    // ADC 最大允许频率为 30MHz，21MHz 是安全且常用的值
    g_adc_handle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    
    // ③ 分辨率：12 位（0~4095）
    // 可选值：12 位（默认）、10 位、8 位、6 位
    // 分辨率越高，精度越高，但转换速度略慢
    g_adc_handle.Init.Resolution = ADC_RESOLUTION_12B;
    
    // ④ 数据对齐方式：右对齐
    // ADC 的 12 位结果可以左对齐或右对齐存放在 16 位寄存器中
    // 右对齐：数据存放在 bit[11:0]，高 4 位为 0
    // 右对齐更方便直接使用结果（0~4095）
    g_adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    
    // ⑤ 扫描模式：禁用（单通道模式）
    // 启用后可以按顺序转换多个通道（需要配合 NbrOfConversion 使用）
    // 本实验只用一个通道（PA1），所以禁用扫描模式
    g_adc_handle.Init.ScanConvMode = DISABLE;
    
    // ⑥ EOC（转换结束）选择：序列转换结束
    // ADC_EOC_SEQ_CONV：在所有规则通道转换完成后，EOC 标志才置位
    // 本实验只转换一个通道，两者效果相同
    g_adc_handle.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    
    // ⑦ 连续转换模式：禁用（单次转换）
    // 启用后，ADC 会连续不断地进行转换（无需重复触发）
    // 禁用时，每次需要软件触发才能启动一次转换
    // 本实验用软件触发，每次按键触发一次转换
    g_adc_handle.Init.ContinuousConvMode = DISABLE;
    
    // ⑧ 规则组转换通道数量：1 个
    // 当 ScanConvMode 启用时，此参数指定要转换的通道数量
    // 本实验只转换 1 个通道，所以设为 1
    g_adc_handle.Init.NbrOfConversion = 1;
    
    // ⑨ 间断转换模式：禁用
    // 启用后，规则组被分成若干子组，每次只转换一个子组
    g_adc_handle.Init.DiscontinuousConvMode = DISABLE;
    
    // ⑩ 间断转换的通道数量：1 个
    // 仅在 DiscontinuousConvMode 启用时有效
    g_adc_handle.Init.NbrOfDiscConversion = 1;
    
    // ? 外部触发源：软件触发
    // ADC_SOFTWARE_START 表示使用软件触发（调用 HAL_ADC_Start 时启动）
    g_adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    
    // ? 外部触发边沿：无（软件触发不需要边沿检测）
    g_adc_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    
    // ? DMA 连续请求：禁用（本实验不使用 DMA）
    g_adc_handle.Init.DMAContinuousRequests = DISABLE;
    
    // ? 调用 HAL 库函数完成 ADC 初始化
    // 此函数会：
    // 1. 检查参数有效性
    // 2. ★ 自动调用 HAL_ADC_MspInit（用户实现，配置 GPIO 和时钟）
    // 3. 写入 ADC 硬件寄存器（CR1、CR2、SQR1~3 等）
    HAL_ADC_Init(&g_adc_handle);
}

/* ====================================================================
   ② HAL 库 ADC MSP 初始化函数（由 HAL_ADC_Init 自动调用）
   ==================================================================== */
/**
 * @brief   HAL库ADC初始化MSP函数
 * @param   hadc: ADC句柄
 * @retval  无
 * @note    此函数由 HAL_ADC_Init 自动调用，不需要用户手动调用
 *          作用是配置 ADC 的底层硬件资源：
 *          1. 使能 ADC 时钟
 *          2. 使能 GPIO 时钟
 *          3. ★ 配置 ADC 引脚为模拟输入模式（关键！）
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    GPIO_InitTypeDef gpio_init_struct = {0};   // GPIO 配置结构体（全部初始化为 0）
    
    // ★ 判断是否是我们要初始化的 ADC 外设
    // 如果项目中有多个 ADC，可以通过这个判断分别配置
    if (hadc->Instance == ADC_ADCX)
    {
        /* --- 步骤 1：使能外设时钟 --- */
        
        // ① 使能 ADC 外设时钟（由 adc.h 中的宏定义）
        // 如果不使能 ADC 时钟，访问 ADC 寄存器会出错
        ADC_ADCX_CLK_ENABLE();
        
        // ② 使能 ADC 引脚的 GPIO 时钟
        // 本实验使用 PA1（属于 GPIOA），所以使能 GPIOA 时钟
        ADC_ADCX_CHY_GPIO_CLK_ENABLE();
        
        /* --- 步骤 2：配置 ADC 引脚 --- */
        
        // ③ 配置引脚为模拟输入模式
        // ★ GPIO_MODE_ANALOG 是 ADC 采样的关键配置！
        // 模拟输入模式下：
        //   - 引脚直接连接到 ADC 内部的采样保持电路
        //   - 数字输入缓冲器被禁用（避免引入噪声）
        //   - 输出驱动器被禁用（引脚只作为输入）
        gpio_init_struct.Pin = ADC_ADCX_CHY_GPIO_PIN;   // 引脚号（PA1）
        gpio_init_struct.Mode = GPIO_MODE_ANALOG;       // ★ 模拟输入模式
        gpio_init_struct.Pull = GPIO_NOPULL;            // 无上下拉（模拟模式不需要）
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;  // 速度（模拟模式无影响）
        
        // ★ 应用配置到对应的 GPIO 端口
        HAL_GPIO_Init(ADC_ADCX_CHY_GPIO_PORT, &gpio_init_struct);
    }
}

/* ====================================================================
   ③ 设置 ADC 通道（动态配置）
   ==================================================================== */
/**
 * @brief   设置ADC通道
 * @param   adc_handle:   ADC句柄指针
 * @param   channel:      ADC通道号（如 ADC_CHANNEL_1）
 * @param   rank:         规则组中的转换顺序（1~16）
 * @param   sampling_time: 采样时间（如 ADC_SAMPLETIME_480CYCLES）
 * @retval  无
 * @note    此函数可以在运行时动态切换 ADC 通道
 *          不频繁切换通道时，也可以把通道配置放在 adc_init 中
 */
void adc_channel_set(ADC_HandleTypeDef *adc_handle, uint32_t channel, uint32_t rank, uint32_t sampling_time)
{
    ADC_ChannelConfTypeDef adc_channel_conf_struct = {0};   // 通道配置结构体（全部初始化为 0）
    
    /* --- 配置 ADC 通道参数 --- */
    
    // ① 通道号（如 ADC_CHANNEL_1 对应 PA1）
    adc_channel_conf_struct.Channel = channel;
    
    // ② 规则组中的转换顺序（1 表示第一个转换）
    // 如果需要多通道扫描，可以设置不同的 rank
    adc_channel_conf_struct.Rank = rank;
    
    // ③ ★ 采样时间（本实验使用 480 个 ADC 时钟周期）
    // 采样时间越长，采样电容充电越充分，精度越高
    // 但速度会变慢，需要根据信号源内阻选择合适的采样时间
    //
    // 常用选项：
    // - ADC_SAMPLETIME_3CYCLES：    最快，适合低阻抗信号源（内阻 < 1kΩ）
    // - ADC_SAMPLETIME_15CYCLES：   较快
    // - ADC_SAMPLETIME_84CYCLES：   中等
    // - ADC_SAMPLETIME_480CYCLES：  ★ 最慢，精度最高（本实验使用）
    adc_channel_conf_struct.SamplingTime = sampling_time;
    
    // ④ 偏移量（本实验不使用）
    // 偏移量用于对 ADC 结果做减法，常用于单端转差分或校准
    adc_channel_conf_struct.Offset = 0;
    
    // ★ 调用 HAL 库函数将配置写入硬件寄存器
    // 此函数会配置 ADC_SQRx（规则序列寄存器）和 ADC_SMPRx（采样时间寄存器）
    HAL_ADC_ConfigChannel(adc_handle, &adc_channel_conf_struct);
}

/* ====================================================================
   ④ 获取单次 ADC 转换结果（阻塞方式）
   ==================================================================== */
/**
 * @brief   获取ADC结果
 * @param   channel: ADC通道（如 ADC_CHANNEL_1）
 * @retval  12 位 ADC 转换结果（0~4095）
 * @note    内部流程：
 *          1. 配置通道（调用 adc_channel_set）
 *          2. 启动 ADC 转换（软件触发）
 *          3. 等待转换完成（轮询 EOC 标志）
 *          4. 读取并返回转换结果
 *          5. ★ 此函数会阻塞 CPU，直到转换完成（超时时间 = HAL_MAX_DELAY）
 */
uint16_t adc_get_result(uint32_t channel)
{
    uint16_t result;   // 存储转换结果
    
    /* --- 步骤 1：配置 ADC 通道 --- */
    // 设置通道、排序位置（Rank=1）和采样时间（480 周期，最高精度）
    adc_channel_set(&g_adc_handle, channel, 1, ADC_SAMPLETIME_480CYCLES);
    
    /* --- 步骤 2：启动 ADC 转换（软件触发） --- */
    // HAL_ADC_Start 会：
    // 1. 使能 ADC 外设（ADEN 位 = 1）
    // 2. 发起软件触发（SWSTART 位 = 1）
    // 3. 开始转换
    HAL_ADC_Start(&g_adc_handle);
    
    /* --- 步骤 3：等待转换完成（阻塞轮询） --- */
    // HAL_ADC_PollForConversion 会：
    // 1. 轮询 EOC（转换结束）标志位
    // 2. 如果超时（HAL_MAX_DELAY = 0xFFFFFFFF），则一直等待
    // 3. 转换完成后返回 HAL_OK，否则返回 HAL_TIMEOUT
    // ★ 注意：此函数会阻塞 CPU，直到转换完成或超时
    HAL_ADC_PollForConversion(&g_adc_handle, HAL_MAX_DELAY);
    
    /* --- 步骤 4：读取并返回转换结果 --- */
    // HAL_ADC_GetValue 会：
    // 1. 读取 ADC_DR（数据寄存器）的值
    // 2. 根据配置的对齐方式（右对齐）返回 12 位结果
    result = HAL_ADC_GetValue(&g_adc_handle);
    
    return result;   // 返回 ADC 值（0~4095）
}

/* ====================================================================
   ⑤ 均值滤波获取 ADC 结果（软件滤波）
   ==================================================================== */
/**
 * @brief   均值滤波获取ADC结果
 * @param   channel: ADC通道
 * @param   times:   均值滤波的采样次数（如 10 次）
 * @retval  滤波后的 ADC 结果（多次采样的平均值）
 * @note    原理：多次采样取平均值，消除随机噪声
 *          优点：结果更稳定，抗干扰能力强
 *          缺点：速度慢（采样次数越多，耗时越长）
 *          适用场景：光敏传感器、温度传感器等易受干扰的信号
 *          不适用场景：需要快速响应的信号（如音频）
 *          
 *          本实验在 DAC 回读中使用，用于稳定显示电压值
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
    // ★ 取平均后，随机噪声被抵消，结果更稳定
    // 例如：采样 10 次，其中几次偏高、几次偏低，平均后趋近真实值
    result = sum_result / times;
    
    return result;   // 返回均值滤波后的结果
}
