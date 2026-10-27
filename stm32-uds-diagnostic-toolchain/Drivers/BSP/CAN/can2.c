#include "./BSP/CAN/can2.h"
#include "./BSP/CAN/common.h"

// 定义CAN2的HAL句柄实体
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
    hcan2.Instance = CAN2;
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
    //连续"邮箱满"的次数
    static uint8_t busy_cnt = 0;
    // 检查数据长度是否超过最大8字节
    if (len > CAN_DLC_MAX) {
        // 超长返回参数错误
        return CAN_ERR_PARAM;
    }
    // 查询空闲发送邮箱数量是否为0
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) == 0) {
        /* 连续 3 次都满 → 说明帧卡死了，强制中止所有挂起的发送 */
        if (++busy_cnt >= 3u) {
            HAL_CAN_AbortTxRequest(&hcan2,
            CAN_TX_MAILBOX0 | CAN_TX_MAILBOX1 | CAN_TX_MAILBOX2);
            busy_cnt = 0;
            printf("TX 卡死，已强制清空邮箱\r\n");
        }
        // 无空闲邮箱返回错误
        return CAN_ERR_NOBOX;
    }
    busy_cnt = 0;
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
    // 从 FIFO0 取出一帧，头信息存入rxHeader，数据存入buf
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
    HAL_NVIC_SetPriority(CAN2_RX0_IRQn, 2, 0);
    // 在 NVIC（Nested Vectored Interrupt Controller，嵌套向量中断控制器）中使能 CAN2 接收中断。
    HAL_NVIC_EnableIRQ(CAN2_RX0_IRQn);
}

/**
 * @brief  CAN2 接收中断服务函数
 * @note   当 CAN2 发生中断时，CPU（Central Processing Unit，中央处理器）自动跳转到这里。
 *         直接调用 HAL（Hardware Abstraction Layer，硬件抽象层）库的通用中断处理函数，
 *         由 HAL 库自动判断中断源、清除标志位，并调用相应的回调函数。
 */
volatile uint32_t g_irq_cnt = 0;
void CAN2_RX0_IRQHandler(void)
{
    g_irq_cnt++;
    // 调用 HAL 库的通用中断处理函数。
    HAL_CAN_IRQHandler(&hcan2);
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

/**
 * @brief  打印 CAN2 的错误状态（TEC/REC/LEC/BOFF 及 HAL 错误码与状态）
 * @note   直接读 ESR 寄存器（Error Status Register），提取：
 *         - TEC：发送错误计数器（Transmit Error Counter，bit7:0）
 *         - REC：接收错误计数器（Receive Error Counter，bit23:16）
 *         - LEC：最后错误代码（Last Error Code，bit6:4）
 *         - BOFF：Bus-Off 标志（bit2）
 *         同时调用 HAL 接口获取累计错误码与当前状态机状态。
 */
void CAN2_PrintStatus(void)
{
    uint32_t esr = hcan2.Instance->ESR;

    printf("TEC=%lu REC=%lu LEC=%lu BOFF=%lu EPV=%lu | ERR=0x%08lX STATE=%d\r\n",
    (unsigned long) ((esr & CAN_ESR_TEC) >> CAN_ESR_TEC_Pos),
    (unsigned long) ((esr & CAN_ESR_REC) >> CAN_ESR_REC_Pos),
    (unsigned long) ((esr & CAN_ESR_LEC) >> CAN_ESR_LEC_Pos),
    (unsigned long) ((esr & CAN_ESR_BOFF) >> CAN_ESR_BOFF_Pos),
    (unsigned long) ((esr & CAN_ESR_EPVF) >> CAN_ESR_EPVF_Pos),
    (unsigned long)HAL_CAN_GetError(&hcan2),
    (int)HAL_CAN_GetState(&hcan2));
    printf("PB12=%lu TSR=0x%08X MSR=0x%08X\r\n",
    (unsigned long) ((GPIOB->IDR >> 12) & 1u),      /* 1=隐性(正常) 0=显性(总线被拉死) */
    (unsigned long)hcan2.Instance->TSR,
    (unsigned long)hcan2.Instance->MSR);
}
