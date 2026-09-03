#include "./BSP/ADC/adc.h"

/* ====================================================================
   全局变量定义
   ==================================================================== */

// ★ 多通道 ADC 句柄（用于扫描模式 + DMA）
// 包含了 ADC 的所有配置参数、状态信息和回调函数指针
// 本实验使用 ADC1，扫描模式采集 PA5 和 PA6 两个通道
ADC_HandleTypeDef g_adc_nch_handle = {0};

// ★ DMA 句柄（用于多通道 ADC 的 DMA 传输）
// 包含了 DMA 的所有配置参数和状态信息
// 本实验使用 DMA2_Stream0_Channel0
DMA_HandleTypeDef g_adc_nch_dma_multi_handle = {0};

// ★ ADC DMA 转换完成标志（由中断回调置位）
// 0 = 未完成，1 = 本次 DMA 传输已完成
// 主循环检测此标志后处理数据
uint8_t g_adc_nch_dma_sta = 0;

// ★ DMA 目标内存基地址（存储 ADC 数据的缓冲区首地址）
// 在 adc_nch_dma_init() 中保存，在 adc_nch_dma_enable() 中使用
uint32_t g_adc_nch_dma_memory_base;


/* ====================================================================
   ① 初始化多通道 ADC DMA 读取
   ==================================================================== */
/**
 * @brief   初始化多通道ADC DMA读取
 * @param   memory_base: 读取目标内存基地址（缓冲区首地址）
 * @retval  无
 * @note    配置 ADC 的核心参数：
 *          - ★ 扫描模式：ENABLE（按顺序转换多个通道）
 *          - ★ 连续转换模式：ENABLE（自动连续运行）
 *          - ★ 通道数量：2（PA5 + PA6）
 *          - ★ DMA 连续请求：ENABLE（每次转换完都触发 DMA）
 */
void adc_nch_dma_init(uint32_t memory_base)
{
    /* --- 步骤 1：配置 ADC 核心参数 --- */
    
    // ① 指定 ADC 外设（ADC1）
    g_adc_nch_handle.Instance = ADC_NCH_DMA_ADCX;
    
    // ② 时钟分频：21MHz
    g_adc_nch_handle.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    
    // ③ 分辨率：12 位
    g_adc_nch_handle.Init.Resolution = ADC_RESOLUTION_12B;
    
    // ④ 数据对齐：右对齐
    g_adc_nch_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    
    // ⑤ ★ 扫描模式：启用（关键！）
    // 启用后，ADC 会按 rank 顺序依次转换多个通道
    // 本实验转换顺序：PA5 → PA6 → PA5 → PA6 ...
    g_adc_nch_handle.Init.ScanConvMode = ENABLE;
    
    // ⑥ EOC 选择：序列转换结束
    g_adc_nch_handle.Init.EOCSelection = ADC_EOC_SEQ_CONV;
    
    // ⑦ ★ 连续转换模式：启用
    // ADC 会持续不断地运行，每次转换完自动触发 DMA
    g_adc_nch_handle.Init.ContinuousConvMode = ENABLE;
    
    // ⑧ ★ 转换通道数量：2 个
    // 必须与扫描的通道总数一致！本实验是 PA5 + PA6 = 2 个
    g_adc_nch_handle.Init.NbrOfConversion = ADC_NCH_DMA_ADCX_CH_NUM;
    
    // ⑨ 间断转换模式：禁用
    g_adc_nch_handle.Init.DiscontinuousConvMode = DISABLE;
    
    // ⑩ 间断转换数量：1
    g_adc_nch_handle.Init.NbrOfDiscConversion = 1;
    
    // ? 外部触发：软件触发
    g_adc_nch_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    
    //外部触发信号是上升沿还是下降沿：无
    g_adc_nch_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    
    // ? ★ DMA 连续请求：启用
    // 每次转换完成都触发 DMA 请求，自动搬运数据
    g_adc_nch_handle.Init.DMAContinuousRequests = ENABLE;
    
    // ? 调用 HAL 库完成 ADC 初始化（会自动调用 HAL_ADC_MspInit）
    HAL_ADC_Init(&g_adc_nch_handle);
    
    // ★ 保存内存基地址
    g_adc_nch_dma_memory_base = memory_base;
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
 *          1. 使能 ADC、GPIO、DMA 时钟
 *          2. 配置两个 ADC 引脚（PA5、PA6）为模拟输入模式
 *          3. 配置 DMA2_Stream0_Channel0
 *          4. 关联 DMA 和 ADC
 *          5. 配置 DMA 中断
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    // GPIO 配置结构体（全部初始化为 0）
    GPIO_InitTypeDef gpio_init_struct = {0};   
    
    // ★ 判断是否是我们要初始化的 ADC 外设
    if (hadc->Instance == ADC_NCH_DMA_ADCX)
    {
        /* --- 步骤 1：使能外设时钟 --- */
        
        // ① 使能 ADC1 外设时钟
        ADC_NCH_DMA_ADCX_CLK_ENABLE();
        
        // ② 使能 PA5（通道 A）的 GPIO 时钟
        ADC_NCH_DMA_ADCX_CHA_GPIO_CLK_ENABLE();
        
        // ③ 使能 PA6（通道 B）的 GPIO 时钟
        // ★ PA5 和 PA6 都属于 GPIOA，实际上只需使能一次即可
        // 但分开写更清晰，便于移植
        ADC_NCH_DMA_ADCX_CHB_GPIO_CLK_ENABLE();
        
        // ④ 使能 DMA2 外设时钟
        // ★ 必须使能 DMA 时钟才能工作！
        ADC_NCH_DMA_ADCX_DMA_CLK_ENABLE();
        
        /* --- 步骤 2：配置 ADC 引脚 --- */
        
        // ★ 配置 PA5 为模拟输入模式（通道 A）
        gpio_init_struct.Pin = ADC_NCH_DMA_ADCX_CHA_GPIO_PIN;   // PA5
        gpio_init_struct.Mode = GPIO_MODE_ANALOG;               // ★ 模拟输入模式
        gpio_init_struct.Pull = GPIO_NOPULL;                    // 无上下拉
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;          // 速度（模拟模式无影响）
        HAL_GPIO_Init(ADC_NCH_DMA_ADCX_CHA_GPIO_PORT, &gpio_init_struct);
        
        // ★ 配置 PA6 为模拟输入模式（通道 B）
        gpio_init_struct.Pin = ADC_NCH_DMA_ADCX_CHB_GPIO_PIN;   // PA6
        gpio_init_struct.Mode = GPIO_MODE_ANALOG;               // ★ 模拟输入模式
        gpio_init_struct.Pull = GPIO_NOPULL;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(ADC_NCH_DMA_ADCX_CHB_GPIO_PORT, &gpio_init_struct);
        
        /* --- 步骤 3：配置 DMA 参数 --- */
        
        // ① 选择 DMA 数据流：DMA2_Stream0
        // ★ ADC1 的 DMA 请求映射到 DMA2_Stream0_Channel0
        g_adc_nch_dma_multi_handle.Instance = ADC_NCH_DMA_ADCX_DMASX;
        
        // ② 选择 DMA 通道：Channel 0
        // ★ ADC1 固定使用 Channel 0（硬件决定）
        g_adc_nch_dma_multi_handle.Init.Channel = ADC_NCH_DMA_ADCX_DMASX_CHY;
        
        // ③ 传输方向：外设 → 内存
        // DMA 从 ADC_DR 寄存器读取数据，存入内存数组
        g_adc_nch_dma_multi_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
        
        // ④ 外设地址不递增（固定读取 ADC_DR）
        g_adc_nch_dma_multi_handle.Init.PeriphInc = DMA_PINC_DISABLE;
        
        // ⑤ 内存地址递增（数组依次填充）
        g_adc_nch_dma_multi_handle.Init.MemInc = DMA_MINC_ENABLE;
        
        // ⑥ 外设数据宽度：16 位（ADC_DR 是 16 位寄存器）
        g_adc_nch_dma_multi_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        
        // ⑦ 内存数据宽度：16 位（与 uint16_t 数组匹配）
        g_adc_nch_dma_multi_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        
        // ⑧ DMA 传输模式：普通模式（传输一次后停止）
        // ★ 每次采集完 length 个数据后停止，需要重新启动
        g_adc_nch_dma_multi_handle.Init.Mode = DMA_NORMAL;
        
        // ⑨ 优先级：最高
        g_adc_nch_dma_multi_handle.Init.Priority = DMA_PRIORITY_VERY_HIGH;
        
        // ⑩ FIFO 模式：禁用（直接模式）
        g_adc_nch_dma_multi_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        
        // ? FIFO 阈值（禁用 FIFO 时无效，但 HAL 要求赋值）
        g_adc_nch_dma_multi_handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL;
        
        // ? 内存突发模式（禁用 FIFO 时无效）
        g_adc_nch_dma_multi_handle.Init.MemBurst = DMA_MBURST_SINGLE;
        
        // ? 外设突发模式（禁用 FIFO 时无效）
        g_adc_nch_dma_multi_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;
        
        // ? 调用 HAL 库函数完成 DMA 初始化
        HAL_DMA_Init(&g_adc_nch_dma_multi_handle);
        
        /* --- 步骤 4：将 DMA 句柄与 ADC 句柄关联（关键步骤） --- */
        // ★ __HAL_LINKDMA 将 ADC 的 DMA_Handle 成员指向 DMA 句柄
        // 这样 HAL_ADC_Start_DMA 就知道用哪个 DMA 通道
        __HAL_LINKDMA(&g_adc_nch_handle, DMA_Handle, g_adc_nch_dma_multi_handle);
        
        /* --- 步骤 5：配置 DMA 中断 --- */
        // ★ 抢占优先级 0（最高），子优先级 0
        HAL_NVIC_SetPriority(ADC_NCH_DMA_ADCX_DMASX_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(ADC_NCH_DMA_ADCX_DMASX_IRQn);
    }
}


/* ====================================================================
   ③ 设置 ADC 通道（动态配置）
   ==================================================================== */
/**
 * @brief   设置ADC通道
 * @param   adc_handle:   ADC句柄指针
 * @param   channel:      ADC通道号（如 ADC_CHANNEL_5）
 * @param   rank:         规则组中的转换顺序（1~16）
 * @param   sampling_time: 采样时间（如 ADC_SAMPLETIME_480CYCLES）
 * @retval  无
 * @note    ★ 在多通道扫描模式中，rank 决定转换顺序
 *          本实验中：rank=1 → PA5 先转，rank=2 → PA6 后转
 */
void adc_channel_set(ADC_HandleTypeDef *adc_handle, uint32_t channel, uint32_t rank, uint32_t sampling_time)
{
    ADC_ChannelConfTypeDef adc_channel_conf_struct = {0};   // 通道配置结构体
    
    // ① 通道号
    adc_channel_conf_struct.Channel = channel;
    
    // ② ★ 转换顺序（rank 越小越先转换）
    // 本实验：rank=1 是 PA5，rank=2 是 PA6
    adc_channel_conf_struct.Rank = rank;
    
    // ③ 采样时间（480 周期，精度最高）
    adc_channel_conf_struct.SamplingTime = sampling_time;
    
    // ④ 偏移量（本实验不使用）
    adc_channel_conf_struct.Offset = 0;
    
    // ★ 调用 HAL 库函数将配置写入硬件寄存器
    HAL_ADC_ConfigChannel(adc_handle, &adc_channel_conf_struct);
}

/* ====================================================================
   ④ 开启多通道 ADC DMA 读取
   ==================================================================== */
/**
 * @brief   开启多通道ADC DMA读取
 * @param   length: DMA读取次数（总采样点数）
 * @retval  无
 * @note    启动前先停止可能正在运行的 DMA
 *          配置两个通道的 rank（PA5 先，PA6 后）
 *          启动 DMA 传输
 *          采集完成后数据排列：buf[0]=PA5, buf[1]=PA6, buf[2]=PA5, buf[3]=PA6 ...
 */
void adc_nch_dma_enable(uint32_t length)
{
    /* --- 步骤 1：停止可能正在进行的 DMA 传输 --- */
    // ★ 先停止，避免冲突
    HAL_ADC_Stop_DMA(&g_adc_nch_handle);
    
    /* --- 步骤 2：配置 ADC 通道（设置扫描顺序） --- */
    // ★ rank=1：PA5 先转换（第 1 个通道）
    adc_channel_set(&g_adc_nch_handle, ADC_NCH_DMA_ADCX_CHA, 1, ADC_SAMPLETIME_480CYCLES);
    
    // ★ rank=2：PA6 后转换（第 2 个通道）
    adc_channel_set(&g_adc_nch_handle, ADC_NCH_DMA_ADCX_CHB, 2, ADC_SAMPLETIME_480CYCLES);
    
    // ★ 扫描顺序已确定：PA5 → PA6 → PA5 → PA6 ...
    //   缓冲区数据排列：buf[0]=PA5, buf[1]=PA6, buf[2]=PA5, buf[3]=PA6 ...
    
    /* --- 步骤 3：启动 ADC + DMA 传输 --- */
    // ★ HAL_ADC_Start_DMA 启动后：
    //   1. ADC1 开始连续扫描转换（PA5 → PA6 → PA5 → PA6 ...）
    //   2. 每次转换完成 → 触发 DMA 请求
    //   3. DMA2_Stream0 从 ADC_DR 读取数据 → 存入缓冲区
    //   4. 内存地址自动递增
    //   5. 采集完 length 个数据后 → DMA 中断 → 回调置位标志
    // 
    // ★ CPU 在采集期间完全自由！
    HAL_ADC_Start_DMA(&g_adc_nch_handle, (uint32_t *)g_adc_nch_dma_memory_base, length);
}



/* ====================================================================
   ⑤ HAL 库 ADC 转换完成回调函数
   ==================================================================== */
/**
 * @brief   HAL库ADC转换完成回调函数
 * @param   hadc: ADC句柄
 * @retval  无
 * @note    此函数由 HAL 库在 DMA 传输完成中断中自动调用
 *          我们在这里标记转换完成，让主循环处理数据
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    // ★ 判断是否是我们要处理的 ADC
    if (hadc->Instance == ADC_NCH_DMA_ADCX)
    {
        // ★ 置位转换完成标志
        // 主循环检测到 sta == 1 后处理缓冲区数据
        // 处理完后记得清零
        g_adc_nch_dma_sta = 1;
    }
}



/* ====================================================================
   ⑥ DMA 中断服务函数（由硬件自动调用）
   ==================================================================== */
/**
 * @brief   DMA中断服务函数
 * @param   无
 * @retval  无
 * @note    此函数由硬件中断自动调用
 *          调用 HAL 库的 DMA 中断处理函数，
 *          它会在传输完成时调用 HAL_ADC_ConvCpltCallback
 */
void ADC_NCH_DMA_ADCX_DMASX_IRQHandler(void)
{
    /* ★ 调用 HAL 库的 DMA 中断处理函数
     *    它会：
     *    1. 读取 DMA 状态寄存器
     *    2. 清除中断标志位
     *    3. 调用用户回调函数 HAL_ADC_ConvCpltCallback
     */
    HAL_DMA_IRQHandler(&g_adc_nch_dma_multi_handle);
}
