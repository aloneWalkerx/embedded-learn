#include "./BSP/DMA/dma.h" 

/* ====================================================================
   全局变量定义
   ==================================================================== */
// DMA 句柄（HAL 库用来管理 DMA 外设的核心结构体）
// 这个句柄包含了 DMA 的所有配置参数和状态信息
DMA_HandleTypeDef g_dma_handle = {0};

// 外部声明 UART1 的句柄（在 usart.c 中定义）
// 我们需要把 DMA 句柄和 UART 句柄绑定，这样 HAL 库才能自动管理
extern UART_HandleTypeDef g_uart1_handle;

/* ====================================================================
   ① DMA 初始化函数
   ==================================================================== */
/**
 * @brief   初始化DMA
 * @param   无
 * @retval  无
 * @note    本实验配置 DMA2_Stream7，用于 USART1 的发送（内存 → 外设）
 *          DMA2 支持内存到内存传输，而 DMA1 不支持，所以优先使用 DMA2
 */
void dma_init(void)
{
    /* --- 步骤 1：使能 DMA2 外设时钟 --- */
    // ★ 必须使能 DMA2 时钟，否则无法访问 DMA2 的寄存器和数据流
    // DMA1 和 DMA2 是独立的两个外设，都有自己独立的时钟开关
    __HAL_RCC_DMA2_CLK_ENABLE();
    
    /* --- 步骤 2：配置 DMA 传输参数 --- */
    
    // ① 选择 DMA 数据流（Stream）
    // ★ DMA2_Stream7：使用 DMA2 的第 7 个数据流
    // STM32F4 的每个 DMA 控制器有 8 个数据流（Stream0~7）
    // 数据流之间可以并行工作，互不干扰
    // 选择 Stream7 是因为 USART1_TX 映射到 DMA2_Stream7（查数据手册可知）
    g_dma_handle.Instance = DMA2_Stream7;
    
    // ② 选择 DMA 通道（Channel）
    // ★ DMA_CHANNEL_4：使用通道 4
    // USART1_TX 固定映射到 DMA2_Stream7_Channel4（硬件决定，不能随意更改）
    // 通道选择必须与数据流匹配，否则 DMA 无法响应外设请求
    g_dma_handle.Init.Channel = DMA_CHANNEL_4;
    
    // ③ 设置传输方向
    // ★ DMA_MEMORY_TO_PERIPH：从内存读取数据，写入外设
    // 本实验用于串口发送：从内存数组 → USART1 的发送数据寄存器（USART_DR）
    // 其他方向：PERIPH_TO_MEMORY（外设→内存）、MEMORY_TO_MEMORY（内存→内存，仅 DMA2）
    g_dma_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
    
    // ④ 设置外设地址递增模式
    // ★ DMA_PINC_DISABLE：外设地址不递增（固定地址）
    // 因为每次都是写入同一个寄存器 USART_DR，地址不变
    // 如果外设是多个寄存器（如 ADC 扫描模式），可以启用递增
    g_dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    
    // ⑤ 设置内存地址递增模式
    // ★ DMA_MINC_ENABLE：内存地址递增（自动指向下一个元素）
    // 发送数组时，每次从内存取一个字节，地址自动 +1
    // 这样就能把整个数组的数据依次发送出去
    g_dma_handle.Init.MemInc = DMA_MINC_ENABLE;
    
    // ⑥ 设置外设数据宽度
    // ★ DMA_PDATAALIGN_BYTE：外设数据宽度为 1 字节（8 位）
    // USART_DR 是 8 位/9 位数据寄存器，使用字节对齐最安全
    // 其他选项：HALFWORD（16位）、WORD（32位）
    g_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    
    // ⑦ 设置内存数据宽度
    // ★ DMA_MDATAALIGN_BYTE：内存数据宽度为 1 字节（8 位）
    // 与发送数组的元素类型 uint8_t 保持一致
    // 如果数组是 uint16_t 类型，这里应改为 HALFWORD
    g_dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    
    // ⑧ 设置 DMA 传输模式
    // ★ DMA_NORMAL：普通模式（传输一次后停止）
    // 发送完指定数量的数据后，DMA 自动停止，不再重新加载
    // 如果需要循环发送（如 ADC 连续采样），可改为 DMA_CIRCULAR（循环模式）
    g_dma_handle.Init.Mode = DMA_NORMAL;
    
    // ⑨ 设置 DMA 通道优先级
    // ★ DMA_PRIORITY_VERY_HIGH：最高优先级
    // 当多个 DMA 流同时请求时，优先级高的先响应
    // 串口发送对实时性要求较高，设置为最高优先级
    // 其他选项：LOW、MEDIUM、HIGH
    g_dma_handle.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    
    // ⑩ 配置 FIFO 模式
    // ★ DMA_FIFOMODE_DISABLE：禁用 FIFO（直接模式）
    // 启用 FIFO 可以缓存数据，提高传输效率，但会增加延迟
    // 简单应用（如串口发送）用直接模式即可
    g_dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    
    // ? FIFO 阈值（仅在 FIFO 启用时有效）
    // ★ DMA_FIFO_THRESHOLD_1QUARTERFULL：FIFO 1/4 满时触发
    // 禁用 FIFO 时此值无效，但 HAL 库要求初始化时赋值
    g_dma_handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL;
    
    // ? 内存突发传输模式（在 FIFO 启用时有效）
    // ★ DMA_MBURST_SINGLE：单次突发（每次传输 1 个数据单元）
    // 禁用 FIFO 时此值无效，但 HAL 库要求初始化时赋值
    g_dma_handle.Init.MemBurst = DMA_MBURST_SINGLE;
    
    // ? 外设突发传输模式（在 FIFO 启用时有效）
    // ★ DMA_PBURST_SINGLE：单次突发
    // 禁用 FIFO 时此值无效，但 HAL 库要求初始化时赋值
    g_dma_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;
    
    // ? 调用 HAL 库函数完成 DMA 初始化
    // 这个函数会把上面的配置写入 DMA 的硬件寄存器
    // 包括：数据流控制寄存器（SCR）、数据流配置寄存器（SCFG）等
    HAL_DMA_Init(&g_dma_handle);
    
    /* --- 步骤 3：将 DMA 句柄与 UART 句柄关联（关键步骤） --- */
    // ★ __HAL_LINKDMA 是一个宏，作用是：
    // 将 UART 句柄的 hdmatx（发送 DMA 句柄）指向 g_dma_handle
    // 这样当 HAL_UART_Transmit_DMA 被调用时，HAL 库就知道使用哪个 DMA 通道
    // 第一个参数：UART 句柄
    // 第二个参数：UART 句柄中的 DMA 发送成员（hdmatx）
    // 第三个参数：我们配置好的 DMA 句柄
    __HAL_LINKDMA(&g_uart1_handle, hdmatx, g_dma_handle);
    
    /* --- 步骤 4：配置 DMA 中断的 NVIC --- */
    // ★ 设置 DMA2_Stream7 中断的优先级：抢占优先级 0（最高），子优先级 0
    // 由于串口发送完成后需要及时通知 CPU，设置为高优先级
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);
    
    // ★ 使能 DMA2_Stream7 中断
    // 当 DMA 传输完成、半传输完成或发生错误时，触发此中断
    // 在中断服务函数中，我们可以执行后续处理（如关闭发送、翻转 LED 等）
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

/* ====================================================================
   ② DMA2 Stream7 中断服务函数
   ==================================================================== */
/**
 * @brief   DMA2 Stream7中断服务函数
 * @param   无
 * @retval  无
 * @note    此函数由硬件中断自动调用，名称需与中断向量表一致
 *          当 DMA 传输完成、半传输完成或发生错误时触发
 *          函数名 "DMA2_Stream7_IRQHandler" 必须与 startup_stm32f407xx.s 中的向量表一致
 */
void DMA2_Stream7_IRQHandler(void)
{
    /* ★ 调用 HAL 库的 DMA 中断处理函数
     *    它会做以下事情：
     *    1. 读取 DMA 的状态寄存器（判断中断来源）
     *    2. 清除对应的中断标志位（防止重复触发）
     *    3. 调用对应的用户回调函数：
     *       - 传输完成 → HAL_DMA_TxCpltCallback()
     *       - 半传输完成 → HAL_DMA_TxHalfCpltCallback()
     *       - 传输错误 → HAL_DMA_ErrorCallback()
     * 
     * ★ 不要在这里直接写业务逻辑，而是统一在回调函数中处理
     *    这样符合 HAL 库的分层设计，也方便代码移植
     */
    HAL_DMA_IRQHandler(&g_dma_handle);
}

/* ====================================================================
   ③ DMA 传输完成回调函数（用户自定义）
   ==================================================================== */
/**
 * @brief   DMA传输完成回调函数（由 HAL 库自动调用）
 * @param   hdma: DMA 句柄指针
 * @retval  无
 * @note    此函数在 DMA 传输完成中断中被调用
 *          可以在这里添加用户代码（如翻转 LED、更新 LCD 显示等）
 */
void HAL_DMA_TxCpltCallback(DMA_HandleTypeDef *hdma)
{
    /* ★ 判断是哪个 DMA 通道完成了传输 */
    if (hdma->Instance == DMA2_Stream7)
    {
        /* ★ 在这里添加传输完成后的处理代码 */
        /* 例如：
         *   1. 翻转 LED1 指示传输完成
         *   2. 在 LCD 上显示 "DMA Send Done!"
         *   3. 触发下一个 DMA 传输
         *   4. 处理接收到的数据
         */
    }
}

/* ====================================================================
   ④ DMA 传输错误回调函数（用户自定义）
   ==================================================================== */
/**
 * @brief   DMA传输错误回调函数（由 HAL 库自动调用）
 * @param   hdma: DMA 句柄指针
 * @retval  无
 * @note    当 DMA 传输过程中发生错误时（如总线冲突、地址越界等），调用此函数
 */
void HAL_DMA_ErrorCallback(DMA_HandleTypeDef *hdma)
{
    /* ★ 判断是哪个 DMA 通道发生了错误 */
    if (hdma->Instance == DMA2_Stream7)
    {
        /* ★ 添加错误处理代码 */
        /* 例如：
         *   1. 打印错误信息到串口
         *   2. 在 LCD 上显示错误提示
         *   3. 重新初始化 DMA
         */
    }
}

