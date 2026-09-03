#include "./BSP/ADC/adc.h"

/* ====================================================================
   全局变量定义
   ==================================================================== */

// ★ ADC 句柄（用于 DMA 模式）
// 包含了 ADC 的所有配置参数、状态信息和回调函数指针
ADC_HandleTypeDef g_adc_dma_handle = {0};


// ★ DMA 句柄（HAL 库用来管理 DMA 外设的核心结构体）
// 包含了 DMA 的所有配置参数和状态信息
DMA_HandleTypeDef g_adc_dma_dma_handle = {0};

// ★ ADC DMA 状态标志
// 0 = 未完成，1 = 转换完成（由中断回调置位）
uint8_t g_adc_dma_sta = 0;

// ★ DMA 目标内存基地址（存储 ADC 数据的缓冲区首地址）
// 在 adc_dma_init() 中保存，在 adc_dma_enable() 中使用
uint32_t g_adc_dma_memory_base;

/* ====================================================================
   ③ 初始化 ADC DMA 读取
   ==================================================================== */
/**
 * @brief   初始化ADC DMA读取
 * @param   memory_base: 读取目标内存基地址（缓冲区数组的首地址）
 * @retval  无
 * @note    配置 ADC 的核心参数，包括：
 *          - 时钟分频（ADC_CLOCK_SYNC_PCLK_DIV4 → 21MHz）
 *          - 分辨率（12 位）
 *          - 数据对齐（右对齐）
 *          - ★ 连续转换模式（ENABLE，启动后自动连续采样）
 *          - ★ DMA 连续请求（ENABLE，每次转换完自动触发 DMA）
 *          - 触发方式（软件触发，启动后自动连续跑）
 */
void adc_dma_init(uint32_t memory_base)
{
    /* --- 步骤 1：配置 ADC 核心参数 --- */
    
    // ① 指定使用哪个 ADC 外设（由 adc.h 中的宏 ADC_DMA_ADCX 决定）
    // 本实验使用 ADC3
    g_adc_dma_handle.Instance = ADC_DMA_ADCX;
    
    // ② 时钟分频：ADC 时钟 = PCLK2 / 4 = 84MHz / 4 = 21MHz
    g_adc_dma_handle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    
    // ③ 分辨率：12 位（0~4095）
    g_adc_dma_handle.Init.Resolution = ADC_RESOLUTION_12B;
    
    // ④ 数据对齐方式：右对齐
    g_adc_dma_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    
    // ⑤ 扫描模式：禁用（单通道模式）
    g_adc_dma_handle.Init.ScanConvMode = DISABLE;
    
    // ⑥ EOC 选择：序列转换结束
    g_adc_dma_handle.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    
    // ⑦ ★ 连续转换模式：启用
    // 与之前的轮询模式不同，DMA 模式下必须启用连续转换
    // 这样 ADC 会一直不停地转换，每次转换完自动触发 DMA 请求
    // CPU 只需要启动一次，后续完全不用管
    g_adc_dma_handle.Init.ContinuousConvMode = ENABLE;
    
    // ⑧ 规则组转换通道数量：1 个
    g_adc_dma_handle.Init.NbrOfConversion = 1;
    
    // ⑨ 间断转换模式：禁用
    g_adc_dma_handle.Init.DiscontinuousConvMode = DISABLE;
    
    // ⑩ 间断转换的通道数量：1 个
    g_adc_dma_handle.Init.NbrOfDiscConversion = 1;
    
    // ? 外部触发源：软件触发
    g_adc_dma_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    
    // ? 外部触发边沿：无
    g_adc_dma_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    
    // ? ★ DMA 连续请求：启用
    // 与之前的轮询模式不同，DMA 模式下必须使能 DMA 连续请求
    // 这样每次 ADC 转换完成后，会自动触发 DMA 请求搬运数据
    // 配合上面的 ContinuousConvMode = ENABLE，实现全自动连续采样
    g_adc_dma_handle.Init.DMAContinuousRequests = ENABLE;
    
    // ? 调用 HAL 库函数完成 ADC 初始化
    // 此函数会自动调用 HAL_ADC_MspInit 配置 GPIO 和 DMA
    HAL_ADC_Init(&g_adc_dma_handle);
    
    // ★ 保存内存基地址，供 adc_dma_enable 使用
    g_adc_dma_memory_base = memory_base;
}


/* ====================================================================
   ① HAL 库 ADC MSP 初始化函数（由 HAL_ADC_Init 自动调用）
   ==================================================================== */
/**
 * @brief   HAL库ADC初始化MSP函数
 * @param   hadc: ADC句柄
 * @retval  无
 * @note    此函数由 HAL_ADC_Init 自动调用，不需要用户手动调用
 *          作用是配置 ADC 的底层硬件资源：
 *          1. 使能 ADC、GPIO、DMA 时钟
 *          2. 配置 ADC 引脚为模拟输入模式
 *          3. 配置 DMA 参数
 *          4. 关联 DMA 和 ADC
 *          5. 配置 DMA 中断
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    GPIO_InitTypeDef gpio_init_struct = {0};   // GPIO 配置结构体（全部初始化为 0）
    
    // ★ 判断是否是我们要初始化的 ADC 外设
    // 如果项目中有多个 ADC，可以通过这个判断分别配置
    if (hadc->Instance == ADC_DMA_ADCX)
    {
        /* --- 步骤 1：使能外设时钟 --- */
        
        // ① 使能 ADC 外设时钟（由 adc.h 中的宏定义）
        // 如果不使能 ADC 时钟，访问 ADC 寄存器会出错
        ADC_DMA_ADCX_CLK_ENABLE();
        
        // ② 使能 ADC 引脚的 GPIO 时钟
        // 本实验使用 PA1（属于 GPIOA），所以使能 GPIOA 时钟
        ADC_DMA_ADCX_CHY_GPIO_CLK_ENABLE();
        
        // ③ 使能 DMA2 外设时钟
        // ★ DMA 必须使能时钟才能工作！
        // DMA1 和 DMA2 是独立的两个外设，都有自己独立的时钟开关
        ADC_DMA_ADCX_DMA_CLK_ENABLE();
        
        /* --- 步骤 2：配置 ADC 引脚（PA1）为模拟输入模式 --- */
        
        gpio_init_struct.Pin = ADC_DMA_ADCX_CHY_GPIO_PIN;   // 引脚号（PA1）
        gpio_init_struct.Mode = GPIO_MODE_ANALOG;           // ★ 模拟输入模式（关键！）
        gpio_init_struct.Pull = GPIO_NOPULL;                // 无上下拉（模拟模式不需要）
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;      // 速度（模拟模式无影响）
        
        // ★ 应用配置到对应的 GPIO 端口
        // 模拟输入模式下，引脚直接连接到 ADC 内部的采样保持电路
        // 数字输入缓冲器和输出驱动器被禁用，避免引入噪声
        HAL_GPIO_Init(ADC_DMA_ADCX_CHY_GPIO_PORT, &gpio_init_struct);
        
        /* --- 步骤 3：配置 DMA 参数 --- */
        
        // ① 选择 DMA 数据流（Stream）
        // ★ 由 adc.h 中的宏 ADC_DMA_ADCX_DMASX 决定（DMA2_Stream1）
        // ADC3 的 DMA 请求映射到 DMA2_Stream1（查数据手册可知）
        g_adc_dma_dma_handle.Instance = ADC_DMA_ADCX_DMASX;
        
        // ② 选择 DMA 通道（Channel）
        // ★ ADC3 固定映射到 DMA2_Stream1_Channel2（硬件决定，不能随意更改）
        g_adc_dma_dma_handle.Init.Channel = ADC_DMA_ADCX_DMASX_CHY;
        
        // ③ 设置传输方向
        // ★ DMA_PERIPH_TO_MEMORY：从外设读取数据，写入内存
        // 本实验用于 ADC 采样：从 ADC_DR 寄存器 → 内存数组
        // 其他方向：MEMORY_TO_PERIPH（内存→外设）、MEMORY_TO_MEMORY（内存→内存）
        g_adc_dma_dma_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
        
        // ④ 设置外设地址递增模式
        // ★ DMA_PINC_DISABLE：外设地址不递增（固定地址）
        // 因为每次都是读取同一个寄存器 ADC_DR，地址不变
        g_adc_dma_dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
        
        // ⑤ 设置内存地址递增模式
        // ★ DMA_MINC_ENABLE：内存地址递增（自动指向下一个元素）
        // 每次读取 ADC 值后，存放到数组的下一个位置
        // 这样就能把多次采样的数据依次存入数组
        g_adc_dma_dma_handle.Init.MemInc = DMA_MINC_ENABLE;
        
        // ⑥ 设置外设数据宽度
        // ★ DMA_PDATAALIGN_HALFWORD：外设数据宽度为 16 位（半字）
        // ADC_DR 是 16 位数据寄存器（12 位有效，高 4 位为 0）
        g_adc_dma_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        
        // ⑦ 设置内存数据宽度
        // ★ DMA_MDATAALIGN_HALFWORD：内存数据宽度为 16 位（半字）
        // 与缓冲区数组的元素类型 uint16_t 保持一致
        // 如果数组是 uint32_t 类型，这里应改为 WORD
        g_adc_dma_dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        
        // ⑧ 设置 DMA 传输模式
        // ★ DMA_NORMAL：普通模式（传输一次后停止）
        // 采集完 length 个数据后，DMA 自动停止
        // 如果需要连续循环采集，可改为 DMA_CIRCULAR（循环模式）
        // 注意：本实验采用 NORMAL 模式，每次需要手动重新启动
        g_adc_dma_dma_handle.Init.Mode = DMA_NORMAL;
        
        // ⑨ 设置 DMA 通道优先级
        // ★ DMA_PRIORITY_VERY_HIGH：最高优先级
        // 当多个 DMA 流同时请求时，优先级高的先响应
        // ADC 采样对实时性要求较高，设置为最高优先级
        // 其他选项：LOW、MEDIUM、HIGH
        g_adc_dma_dma_handle.Init.Priority = DMA_PRIORITY_VERY_HIGH;
        
        // ⑩ FIFO 模式：禁用
        // ★ DMA_FIFOMODE_DISABLE：禁用 FIFO（直接模式）
        // 启用 FIFO 可以缓存数据，提高传输效率，但会增加延迟
        // ADC 采样用直接模式即可
        g_adc_dma_dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        
        // ? FIFO 阈值（仅在 FIFO 启用时有效）
        // ★ DMA_FIFO_THRESHOLD_1QUARTERFULL：FIFO 1/4 满时触发
        g_adc_dma_dma_handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL;
        
        // ? 内存突发传输模式（在 FIFO 启用时有效）
        // ★ DMA_MBURST_SINGLE：单次突发（每次传输 1 个数据单元）
        g_adc_dma_dma_handle.Init.MemBurst = DMA_MBURST_SINGLE;
        
        // ? 外设突发传输模式（在 FIFO 启用时有效）
        // ★ DMA_PBURST_SINGLE：单次突发
        g_adc_dma_dma_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;
        
        // ? 调用 HAL 库函数完成 DMA 初始化
        // 这个函数会把上面的配置写入 DMA 的硬件寄存器
        HAL_DMA_Init(&g_adc_dma_dma_handle);
        
        /* --- 步骤 4：将 DMA 句柄与 ADC 句柄关联（关键步骤） --- */
        // ★ __HAL_LINKDMA 是一个宏，作用是：
        // 将 ADC 句柄的 DMA_Handle 成员指向 g_adc_dma_dma_handle
        // 这样当 HAL_ADC_Start_DMA 被调用时，HAL 库就知道使用哪个 DMA 通道
        // 第一个参数：ADC 句柄
        // 第二个参数：ADC 句柄中的 DMA 成员（DMA_Handle）
        // 第三个参数：我们配置好的 DMA 句柄
        __HAL_LINKDMA(&g_adc_dma_handle, DMA_Handle, g_adc_dma_dma_handle);
        
        /* --- 步骤 5：配置 DMA 中断的 NVIC --- */
        // ★ 设置 DMA2_Stream1 中断的优先级：抢占优先级 0（最高），子优先级 0
        // 由于 ADC 采样完成后需要及时通知 CPU，设置为高优先级
        HAL_NVIC_SetPriority(ADC_DMA_ADCX_DMASX_IRQn, 0, 0);
        
        // ★ 使能 DMA2_Stream1 中断
        // 当 DMA 传输完成、半传输完成或发生错误时，触发此中断
        HAL_NVIC_EnableIRQ(ADC_DMA_ADCX_DMASX_IRQn);
    }
}

/* ====================================================================
   ② 设置 ADC 通道（动态配置）
   ==================================================================== */
/**
 * @brief   设置ADC通道
 * @param   adc_handle:   ADC句柄指针
 * @param   channel:      ADC通道号（如 ADC_CHANNEL_1）
 * @param   rank:         规则组中的转换顺序（1~16）
 * @param   sampling_time: 采样时间（如 ADC_SAMPLETIME_480CYCLES）
 * @retval  无
 */
void adc_channel_set(ADC_HandleTypeDef *adc_handle, uint32_t channel, uint32_t rank, uint32_t sampling_time)
{
    ADC_ChannelConfTypeDef adc_channel_conf_struct = {0};   // 通道配置结构体（全部初始化为 0）
    
    /* --- 配置 ADC 通道参数 --- */
    
    // ① 通道号（如 ADC_CHANNEL_1 对应 PA1）
    adc_channel_conf_struct.Channel = channel;
    
    // ② 规则组中的转换顺序（1 表示第一个转换）
    adc_channel_conf_struct.Rank = rank;
    
    // ③ 采样时间（本实验使用 480 个 ADC 时钟周期，精度最高）
    adc_channel_conf_struct.SamplingTime = sampling_time;
    
    // ④ 偏移量（本实验不使用）
    adc_channel_conf_struct.Offset = 0;
    
    // ★ 调用 HAL 库函数将配置写入硬件寄存器
    HAL_ADC_ConfigChannel(adc_handle, &adc_channel_conf_struct);
}



/* ====================================================================
   ④ 开启 ADC DMA 读取
   ==================================================================== */
/**
 * @brief   开启ADC DMA读取
 * @param   length: DMA 读取次数（缓冲区大小）
 * @retval  无
 * @note    启动后：
 *          1. 先停止可能正在运行的 DMA 传输（避免冲突）
 *          2. 设置 ADC 通道（采样时间 480 周期）
 *          3. 启动 ADC + DMA 传输
 *          4. 硬件自动连续采样，DMA 自动搬运数据到缓冲区
 *          5. 采样完成后触发 DMA 中断
 */
void adc_dma_enable(uint32_t length)
{
    /* --- 步骤 1：停止可能正在进行的 DMA 传输 --- */
    // ★ 先停止，确保 DMA 处于空闲状态
    // 这样可以避免重复启动导致的数据错乱
    HAL_ADC_Stop_DMA(&g_adc_dma_handle);
    
    /* --- 步骤 2：设置 ADC 通道 --- */
    // 配置通道 1、排序位置 1、采样时间 480 周期
    adc_channel_set(&g_adc_dma_handle, ADC_DMA_ADCX_CHY, 1, ADC_SAMPLETIME_480CYCLES);
    
    /* --- 步骤 3：启动 ADC + DMA 传输 --- */
    // ★ HAL_ADC_Start_DMA 是核心操作！
    // 参数 1：ADC 句柄
    // 参数 2：DMA 目标内存地址（缓冲区首地址）
    // 参数 3：要采集的数据个数（缓冲区大小）
    // 
    // ★ 执行这条语句后，ADC + DMA 协同工作：
    //   1. ADC 开始连续转换（ContinuousConvMode = ENABLE）
    //   2. 每次转换完成 → 触发 DMA 请求
    //   3. DMA 从 ADC_DR 读取数据 → 存入缓冲区
    //   4. 内存地址自动递增（MemInc = ENABLE）
    //   5. 采集完 length 个数据后，触发 DMA 中断
    //   6. 中断回调中置位 g_adc_dma_sta = 1
    // 
    // ★ 在 ADC + DMA 采集期间，CPU 完全自由！
    //    可以处理其他任务（如 LED 闪烁、按键扫描、LCD 刷新等）
    HAL_ADC_Start_DMA(&g_adc_dma_handle, (uint32_t *)g_adc_dma_memory_base, length);
}

/* ====================================================================
   ⑤ HAL 库 ADC 转换完成回调函数
   ==================================================================== */
/**
 * @brief   HAL库ADC转换完成回调函数
 * @param   hadc: ADC句柄
 * @retval  无
 * @note    此函数由 HAL 库在 DMA 传输完成中断中自动调用
 *          我们在这里标记转换完成，让主循环知道数据已经准备好了
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    // ★ 判断是否是我们要处理的 ADC
    if (hadc->Instance == ADC_DMA_ADCX)
    {
        // ★ 置位转换完成标志
        // 主循环检测到 g_adc_dma_sta == 1 后，就可以处理缓冲区中的数据了
        // 处理完后记得将 g_adc_dma_sta 清零
        g_adc_dma_sta = 1;
    }
}

/* ====================================================================
   ⑥ DMA 中断服务函数（由硬件自动调用）
   ==================================================================== */
/**
 * @brief   DMA中断服务函数
 * @param   无
 * @retval  无
 * @note    此函数由硬件中断自动调用，名称需与中断向量表一致
 *          当 DMA 传输完成、半传输完成或发生错误时触发
 *          函数名 "ADC_DMA_ADCX_DMASX_IRQHandler" 由 adc.h 中的宏定义
 */
void ADC_DMA_ADCX_DMASX_IRQHandler(void)
{
    /* ★ 调用 HAL 库的 DMA 中断处理函数
     *    它会做以下事情：
     *    1. 读取 DMA 的状态寄存器（判断中断来源）
     *    2. 清除对应的中断标志位（防止重复触发）
     *    3. 调用对应的用户回调函数：
     *       - 传输完成 → HAL_ADC_ConvCpltCallback()
     *       - 半传输完成 → HAL_ADC_ConvHalfCpltCallback()
     *       - 传输错误 → HAL_ADC_ErrorCallback()
     * 
     * ★ 不要在这里直接写业务逻辑，而是统一在回调函数中处理
     *    这样符合 HAL 库的分层设计，也方便代码移植
     */
    HAL_DMA_IRQHandler(&g_adc_dma_dma_handle);
}
