#include "./BSP/CAN/can.h"
#include "stdio.h"
#include "./BSP/CAN/can_test.h"

volatile uint16_t g_rx_id = 0;
volatile uint8_t g_rx_len = 0;
volatile uint8_t g_rx_buf[8];
volatile uint32_t g_rx_cnt = 0;

// 定义CAN2的HAL句柄实体（供全工程引用）
CAN_HandleTypeDef hcan2;

/**
 * @brief  CAN2初始化（时钟、引脚、位时序、启动）
 * @param  baud_sel: 波特率选择（CAN_500K / CAN_250K / CAN_125K）
 * @param  loopback: 非0表示回环模式，0表示正常模式
 */
void CAN2_Init(uint8_t baud_sel, uint8_t loopback)
{
    // 预分频值暂存变量
    uint32_t presc;

    /* APB1 = 42MHz */
    // 根据波特率选择确定预分频值
    switch (baud_sel) {
        // 250kbps 对应预分频12
    case CAN_250K: presc = CAN_PRESCALER_250K; break;
        // 125kbps 对应预分频24
    case CAN_125K: presc = CAN_PRESCALER_125K; break;
        // 其他情况默认500kbps，预分频6
    default: presc = CAN_PRESCALER_500K; break;
    }
    // 指定外设实例为CAN2
    hcan2.Instance = CAN_CANX;
    // 设置预分频值
    hcan2.Init.Prescaler = presc;
    // 回环或正常模式
    hcan2.Init.Mode = loopback ? CAN_MODE_LOOPBACK : CAN_MODE_NORMAL;
    // 同步跳转宽度（1tq）
    hcan2.Init.SyncJumpWidth = CAN_SJW_TQ;
    // 时间段1（11tq）
    hcan2.Init.TimeSeg1 = CAN_BS1_TQ;
    // 时间段2（2tq）
    hcan2.Init.TimeSeg2 = CAN_BS2_TQ;
    // 禁用时间触发模式
    hcan2.Init.TimeTriggeredMode = DISABLE;
    // 使能Bus-Off自动恢复
    hcan2.Init.AutoBusOff = ENABLE;
    // 禁用自动唤醒
    hcan2.Init.AutoWakeUp = DISABLE;
    // 使能自动重传
    hcan2.Init.AutoRetransmission = ENABLE;
    // 接收FIFO不锁定（溢出时新报文覆盖旧报文）
    hcan2.Init.ReceiveFifoLocked = DISABLE;
    // 发送按请求顺序（非ID优先级）
    hcan2.Init.TransmitFifoPriority = DISABLE;

    // 调用HAL初始化CAN2（内部会回调MspInit）
    if (HAL_CAN_Init(&hcan2) != HAL_OK) {
        // 初始化失败打印提示
        printf("CAN2 init FAIL\r\n");
        // 直接返回
        return;
    }
    // 启动CAN2（退出初始化模式进入正常/回环工作）
    if (HAL_CAN_Start(&hcan2) != HAL_OK) {
        // 启动失败打印提示
        printf("CAN2 start FAIL\r\n");
        // 直接返回
        return;
    }

    // 打印APB1时钟频率（用于核对位时序）
    printf("PCLK1=%lu\r\n", (unsigned long)HAL_RCC_GetPCLK1Freq());
    // 打印初始化成功信息：模式与速率
    printf("CAN2 ready (HAL), %s @%s\r\n",
    // 模式字符串：回环/正常
    loopback ? "loopback" : "normal",
    // 速率字符串选择：250k
    (baud_sel == CAN_250K) ? "250k" :
    // 否则125k/默认500k
    (baud_sel == CAN_125K) ? "125k" : "500k");
}

/**
 * @brief  CAN外设MSP初始化（HAL在HAL_CAN_Init内部自动回调）
 * @param  hcan: CAN句柄指针
 * @note   负责时钟使能与GPIO复用配置，仅对CAN2生效
 */
void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{
    // 定义GPIO初始化结构体并清零
    GPIO_InitTypeDef gpio_init = {0};
    // 判断是否为CAN2实例
    if (hcan->Instance != CAN2) {
        // 不是CAN2则直接返回（本驱动只处理CAN2）
        return;
    }
    // 使能GPIOB时钟（PB12/PB13需要）
    CAN_GPIO_PORT_CLK_ENABLE();
    //使能CAN1时钟（CAN2从控制器依赖CAN1主控制器的时钟）,只用 CAN2 也必须开
    CAN_CAN1_CLK_ENABLE();
    // 使能CAN2时钟
    CAN_CAN2_CLK_ENABLE();
    // 同时配置接收和发送引脚,PB12=RX, PB13=TX
    gpio_init.Pin = CAN_GPIO_TX_PIN | CAN_GPIO_RX_PIN;
    // 复用推挽输出模式
    gpio_init.Mode = GPIO_MODE_AF_PP;
    //上拉（CAN RX通常需要上拉）
    gpio_init.Pull = GPIO_PULLUP;
    // 最高速度档（对应OSPEEDR=11）
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    // 复用功能选择AF9（CAN2）
    gpio_init.Alternate = CAN_GPIO_AF;
    // 按上述参数初始化GPIOB
    HAL_GPIO_Init(GPIOB, &gpio_init);
}

/**
 * @brief  CAN2发送一帧标准数据帧
 * @param  id:  标准帧ID（11位）
 * @param  buf: 数据缓冲区指针
 * @param  len: 数据长度（0~8）
 * @return CAN_OK 成功；CAN_ERR_PARAM 参数错误；CAN_ERR_NOBOX 无空闲邮箱或发送失败
 */
uint8_t CAN2_SendStd(uint16_t id, uint8_t *buf, uint8_t len)
{
    // 定义发送报文头结构体
    CAN_TxHeaderTypeDef txHeader;
    // 用于返回选中的发送邮箱编号
    uint32_t mailbox;
    // 检查数据长度是否超过最大8字节
    if (len > CAN_DLC_MAX) {
        // 超长返回参数错误
        return CAN_ERR_PARAM;
    }
    // 查询空闲发送邮箱数量是否为0
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) == 0) {
        // 无空闲邮箱返回错误
        return CAN_ERR_NOBOX;
    }
    // 标准ID（与0x7FF掩码，确保11位有效）
    txHeader.StdId = (uint32_t) (id & CAN_STD_ID_MASK);
    // 扩展ID置0（标准帧不用）
    txHeader.ExtId = 0;
    // 标识符类型：标准帧
    txHeader.IDE = CAN_ID_STD;
    // 帧类型：数据帧（非远程帧）
    txHeader.RTR = CAN_RTR_DATA;
    // 数据长度（0~8）
    txHeader.DLC = len;
    // 不启用发送时间戳
    txHeader.TransmitGlobalTime = DISABLE;

    // 将报文加入发送邮箱并启动发送
    if (HAL_CAN_AddTxMessage(&hcan2, &txHeader, buf, &mailbox) != HAL_OK) {
        // 添加失败返回错误
        return CAN_ERR_NOBOX;
    }

    // 发送请求成功返回OK
    return CAN_OK;
}

/**
 * @brief  从 CAN2 的 FIFO0 读取一帧数据（轮询方式）
 * @param  id:  输出参数，返回该帧的 ID（标准帧返 StdId，扩展帧返 ExtId）
 * @param  buf: 输出参数，接收数据缓冲区（调用者需保证至少 8 字节）
 * @param  len: 输出参数，返回该帧的数据长度（DLC）
 * @return CAN_OK 成功读到一帧；CAN_ERR_EMPTY FIFO0 为空或读取失败
 */
uint8_t CAN2_RecvStd(uint16_t *id, uint8_t *buf, uint8_t *len)
{
    // 定义接收报文头结构体（存放ID、帧类型、长度等）
    CAN_RxHeaderTypeDef rxHeader;
    // 查询 FIFO0 中已填充的报文数量
    if (HAL_CAN_GetRxFifoFillLevel(&hcan2, CAN_RX_FIFO0) == 0) {
        // 数量为0说明没有新报文，返回空错误
        return CAN_ERR_EMPTY;
    }
    // 从 FIFO0 取出一帧，头信息存入rx，数据存入buf
    if (HAL_CAN_GetRxMessage(&hcan2, CAN_RX_FIFO0, &rxHeader, buf) != HAL_OK) {
        // 读取失败（极少见，如FIFO被抢空），返回空错误
        return CAN_ERR_EMPTY;
    }
    // 判断该帧是标准帧（11位ID）还是扩展帧（29位ID）
    if (rxHeader.IDE == CAN_ID_STD) {
        // 标准帧：把 StdId 输出给调用者
        *id = (uint16_t)rxHeader.StdId;
    } else {
        // 扩展帧：把 ExtId 截断为16位输出（注意会丢失高位）
        *id = (uint16_t)rxHeader.ExtId;
    }
    // 输出数据长度（DLC，0~8）
    *len = (uint8_t)rxHeader.DLC;
    // 成功读取一帧，返回OK
    return CAN_OK;
}

/**
 * @brief  配置 CAN2 滤波器为"全部放行"（调试用，接收所有ID）
 * @note   CAN2 是 CAN1 的从控制器，滤波器组从 bank14 开始，
 *         必须通过 SlaveStartFilterBank 告诉 HAL 分界点。
 *         32位掩码模式下，掩码全0表示不关心ID任何位 => 全部接收。
 */
void CAN2_Filter_AcceptAll(void)
{
    // 定义滤波器配置结构体并清零（避免未初始化字段）
    CAN_FilterTypeDef canFilter = {0};
    // 指定使用的滤波器组编号（14），CAN2 从 bank 14 起
    canFilter.FilterBank = CAN_FILTER_BANK;
    // 滤波模式：ID掩码模式（另一种是ID列表模式）
    canFilter.FilterMode = CAN_FILTERMODE_IDMASK;
    // 滤波尺度：32位（另一种是16位）
    canFilter.FilterScale = CAN_FILTERSCALE_32BIT;
    // ID高16位 = 0（CAN_FILTER_ALL 定义为0x0000）
    canFilter.FilterIdHigh = CAN_FILTER_ALL;
    // ID低16位 = 0
    canFilter.FilterIdLow = CAN_FILTER_ALL;
    // 掩码高16位 = 0 => 不比较ID高16位
    canFilter.FilterMaskIdHigh = CAN_FILTER_ALL;
    // 掩码低16位 = 0 => 不比较ID低16位（合起来：全部放行）
    canFilter.FilterMaskIdLow = CAN_FILTER_ALL;
    // 匹配到的报文放入 FIFO0
    canFilter.FilterFIFOAssignment = CAN_RX_FIFO0;
    // 使能该滤波器
    canFilter.FilterActivation = ENABLE;
    // 从控制器（CAN2）的滤波器起始bank = 14
    canFilter.SlaveStartFilterBank = CAN_SLAVE_START_BANK;
    // 调用HAL把上述配置写入CAN2滤波器
    if (HAL_CAN_ConfigFilter(&hcan2, &canFilter) != HAL_OK) {
        // 配置失败打印提示（调试用）
        printf("filter FAIL\r\n");
    }
}

/**
 * @brief  配置并使能 CAN2 接收中断
 * @note   使能 FIFO0（First In First Out，先进先出）消息挂起（Pending）中断，
 *         并在 NVIC（Nested Vectored Interrupt Controller，嵌套向量中断控制器）中设置优先级并开启。
 *         告诉硬件"如果 FIFO0 邮箱里收到新消息了，记得敲门（触发中断）叫我"，并接通门铃。
 */
void CAN2_IRQ_Enable(void)
{
    // 激活 CAN2 的接收 FIFO0 挂起（Pending）中断。
    if (HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        // 如果配置失败，打印错误信息并退出
        printf("CAN2 irq FAIL\r\n");
        return;
    }
    // 设置 CAN2 接收中断的优先级为 2（数字越小优先级越高），抢占优先级为 0。
    HAL_NVIC_SetPriority(CAN_CAN2_RX_IRQn, 2, 0);
    // 在 NVIC（Nested Vectored Interrupt Controller，嵌套向量中断控制器）中使能 CAN2 接收中断。
    HAL_NVIC_EnableIRQ(CAN_CAN2_RX_IRQn);
}

/**
 * @brief  CAN2 接收中断服务函数
 * @note   当 CAN2 发生中断时，CPU（Central Processing Unit，中央处理器）自动跳转到这里。
 *         直接调用 HAL（Hardware Abstraction Layer，硬件抽象层）库的通用中断处理函数，
 *         由 HAL 库自动判断中断源、清除标志位，并调用相应的回调函数。
 */
void CAN_CAN2_RX_IRQHandler(void)
{
    // 调用 HAL 库的通用中断处理函数。
    HAL_CAN_IRQHandler(&hcan2);
}

/**
 * @brief  接收 FIFO0 挂起回调函数（Callback，回调函数）
 * @param  hcan (CAN_HandleTypeDef *)：触发此回调函数的 CAN 句柄指针，包含了当前 CAN 控制器的状态。
 * @note   这是 HAL 库在底层处理完中断后自动调用的函数。
 *         将接收到的 ID（Identifier，标识符）、DLC（Data Length Code，数据长度编码）和 DATA（数据）提取出来，
 *         保存到全局变量中供主程序使用，并对数据长度做越界保护（最大 8 字节）。
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    //定义一个结构体，用来装收到的ID、长度、帧类型等
    CAN_RxHeaderTypeDef rxHeader;
    // 定义一个 8 字节的数组，用来装数据
    uint8_t buf[8], i;

    // 从 CAN 控制器的 FIFO0 中把消息读出来，存放到 rxHeader 结构体和 buf 数组中
    // 如果读取失败（比如没有消息或者硬件错误），直接退出回调函数
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, buf) != HAL_OK) {
        return;
    }

    // 获取发送方的 ID（Identifier，标识符）。
    // 判断是标准帧还是扩展帧：如果是标准帧(CAN_ID_STD)取 StdId(11位)，否则取 ExtId(29位)。最后强转成 16 位保存。
    g_rx_id = (uint16_t) (rxHeader.IDE == CAN_ID_STD ? rxHeader.StdId : rxHeader.ExtId);

    // 获取数据的长度 DLC（Data Length Code，数据长度编码）
    g_rx_len = (uint8_t)rxHeader.DLC;

    // 安全检查：CAN 协议规定最多 8 个字节，如果读到的长度大于 8，强制改成 8，防止数组越界（内存踩踏）
    if (g_rx_len > 8u) {
        g_rx_len = 8u;
    }

    // 把收到的数据从临时缓冲区 buf 复制到全局变量 g_rx_buf 中，方便主程序使用
    for (i = 0; i < g_rx_len; i++) {
        g_rx_buf[i] = buf[i];
    }

    // 接收计数器加 1，用于统计一共收到了多少条消息
    g_rx_cnt++;
}

/**
 * @brief  配置 CAN2 滤波器为 32 位列表模式，放行两个标准 ID
 * @param  id1 (uint16_t)：允许通过的第 1 个标准帧 ID（11 位，最大 0x7FF）
 * @param  id2 (uint16_t)：允许通过的第 2 个标准帧 ID（11 位，最大 0x7FF）
 * @note   列表模式（IDLIST）下，ID 寄存器存放的是"要匹配的 ID 本身"，而不是掩码，
 *         硬件会逐一比对，只有等于 id1 或 id2 的帧才会被放进 FIFO0。
 *         32 位尺度下，一个滤波器组占用 2 个 32 位寄存器（FR1、FR2），可放 2 个标准 ID。
 *         CAN2 是从控制器，滤波器组从 bank14 开始，需通过 SlaveStartFilterBank 告知 HAL。
 */
void CAN2_Filter_List2(uint16_t id1, uint16_t id2)
{
    // 定义滤波器配置结构体并清零（避免未初始化字段影响配置）
    CAN_FilterTypeDef canFilter = {0};

    // 指定使用的滤波器组编号（CAN2 从 bank14 起）
    canFilter.FilterBank = CAN_FILTER_BANK;
    // 滤波模式：ID 列表模式（另一种是 ID 掩码模式 IDMASK）
    canFilter.FilterMode = CAN_FILTERMODE_IDLIST;
    // 滤波尺度：32 位（一个 bank 可放 2 个标准 ID 或 1 个扩展 ID）
    canFilter.FilterScale = CAN_FILTERSCALE_32BIT;
    // 第 1 个标准 ID 写入 FR1 高位：标准 ID 占 11 位，需左移 5 位对齐到 FR1[31:21]
    canFilter.FilterIdHigh = (uint16_t) (((uint32_t) (id1 & CAN_STD_ID_MASK)) << 5); /* FR1 = id1 << 21 */
    // FR1 低 16 位不使用（标准 ID 只占高 16 位的一部分），填 0
    canFilter.FilterIdLow = CAN_FILTER_ALL;
    // 第 2 个标准 ID 写入 FR2 高位：同样左移 5 位对齐
    canFilter.FilterMaskIdHigh = (uint16_t) (((uint32_t) (id2 & CAN_STD_ID_MASK)) << 5); /* FR2 = id2 << 21 */
    // FR2 低 16 位不使用，填 0
    canFilter.FilterMaskIdLow = CAN_FILTER_ALL;
    // 匹配到的报文放入 FIFO0
    canFilter.FilterFIFOAssignment = CAN_RX_FIFO0;
    // 使能该滤波器（列表模式下，ID 匹配即放行）
    canFilter.FilterActivation = ENABLE;
    // 从控制器（CAN2）的滤波器起始 bank = 14，告知 HAL 分界点
    canFilter.SlaveStartFilterBank = CAN_SLAVE_START_BANK;

    // 调用 HAL 把上述配置写入 CAN2 滤波器组
    if (HAL_CAN_ConfigFilter(&hcan2, &canFilter) != HAL_OK) {
        // 配置失败打印提示（调试用）
        printf("filter FAIL\r\n");
    }
}
