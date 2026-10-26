#include "./BSP/CAN/can1.h"
#include "./BSP/CAN/common.h"

//定义CAN1的HAL句柄实体
CAN_HandleTypeDef hcan1;

/**
 * @brief  CAN1初始化（时钟、引脚、位时序、启动）
 * @param  baud_sel: 波特率选择（CAN_500K / CAN_250K / CAN_125K）
 * @param  loopback: 非0表示回环模式，0表示正常模式
 */
void CAN1_Init(uint8_t baud_sel, uint8_t loopback)
{
    //波特率定义
    uint32_t presc;
    //根据对应的波特率设置对应的预分频
    switch (baud_sel) {
        //250kbps对应的预分频实12
    case CAN_250K: presc = CAN_PRESCALER_250K; break;
        //125kbps对应的预分频实24
    case CAN_125K: presc = CAN_PRESCALER_125K; break;
        //否则预分频为6
    default: presc = CAN_PRESCALER_500K; break;
    }

    //制定外设实例为CAN1
    hcan1.Instance = CAN1;
    //设置预分频值
    hcan1.Init.Prescaler = presc;
    //设置环回模式或正常模式
    hcan1.Init.Mode = loopback ? CAN_MODE_LOOPBACK : CAN_MODE_NORMAL;
    //同步跳转宽度
    hcan1.Init.SyncJumpWidth = CAN_SJW_TQ;
    //时间段1（11tq）
    hcan1.Init.TimeSeg1 = CAN_BS1_TQ;
    //时间段2（2tq）
    hcan1.Init.TimeSeg2 = CAN_BS2_TQ;
    //禁用时间触发模式
    hcan1.Init.TimeTriggeredMode = DISABLE;
    //使能Bus-Off自动恢复
    hcan1.Init.AutoBusOff = ENABLE;
    //禁用自动唤醒
    hcan1.Init.AutoWakeUp = DISABLE;
    //使能自动启发
    hcan1.Init.AutoRetransmission = ENABLE;
    //接收FIFO时不锁定（即FIFO满时新数据会覆盖掉最的旧数据）
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    //发送优先级（ENABLE的情况下按ID优先级），此处DISABLE，非ID优先级
    hcan1.Init.TransmitFifoPriority = DISABLE;
    //根据对应的参数初始化CAN1,会自动调用MspInit
    if (HAL_CAN_Init(&hcan1) != HAL_OK) {
        //初始化失败则打印对应的错误提示信息
        printf("CAN1 init FAIL\r\n");
        //直接返回
        return;
    }
    //启动CAN1(退出初始化模式进入环回/或正常模式)
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        //启动失败则打印对应的提示信息
        printf("CAN1 start FAIL\r\n");
        //直接返回
        return;
    }
    //打印成功的对应提示信息
    printf("CAN1 ready (HAL), %s @500k\r\n", loopback ? "loopback" : "normal");
}

/**
 * @brief  CAN1发送一帧标准数据帧
 * @param  id:  标准帧ID（11位）
 * @param  buf: 数据缓冲区指针
 * @param  len: 数据长度（0~8）
 * @return CAN_OK 成功；CAN_ERR_PARAM 参数错误；CAN_ERR_NOBOX 无空闲邮箱或发送失败
 */
uint8_t CAN1_SendStd(uint16_t id, uint8_t *buf, uint8_t len)
{
    //定义发送报文头结构体
    CAN_TxHeaderTypeDef txHeader;
    //用于返回选中的发送邮箱编号
    uint32_t mailbox;
    //检查数据长度是否超过最大8字节
    if (len > CAN_DLC_MAX) {
        //超过则返回错误代码
        return CAN_ERR_PARAM;
    }
    //查询是否有空闲发送邮箱，为0时则代表没有
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0) {
        //返回无空闲邮箱错误代码
        return CAN_ERR_NOBOX;
    }

    //标准ID与上0X7FF掩码（0x011111111111）,确保11位有效
    txHeader.StdId = (uint32_t) (id & CAN_STD_ID_MASK);
    //扩展ID设置为01，标准帧不使用
    txHeader.ExtId = 0;
    //标识符类型：标准帧
    txHeader.IDE = CAN_ID_STD;
    //帧类型：数据帧
    txHeader.RTR = CAN_RTR_DATA;
    //数据长度
    txHeader.DLC = len;
    //不启用发送时间戳
    txHeader.TransmitGlobalTime = DISABLE;

    //将报文加入发送邮箱并启动发送
    if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, buf, &mailbox) != HAL_OK) {
        //添加失败则返回对应的错误码
        return CAN_ERR_NOBOX;
    }
    //返回发送成功的状态码
    return CAN_OK;
}

/**
 * @brief  从 CAN1 的 FIFO0 读取一帧数据（轮询方式）
 * @param  id:  输出参数，返回该帧的 ID（标准帧返 StdId，扩展帧返 ExtId）
 * @param  buf: 输出参数，接收数据缓冲区（调用者需保证至少 8 字节）
 * @param  len: 输出参数，返回该帧的数据长度（DLC）
 * @return CAN_OK 成功读到一帧；CAN_ERR_EMPTY FIFO0 为空或读取失败
 */
uint8_t CAN1_RecvStd(uint16_t *id, uint8_t *buf, uint8_t *len)
{
    //定义接收报文头结构体（存放ID，帧类型，长度等）
    CAN_RxHeaderTypeDef rxHeader;
    //查询FIFO0中是否有数据填充
    if (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) == 0) {
        //没有数据则返回空代码
        return CAN_ERR_EMPTY;
    }
    //从FIFO0中取出一帧数据，头信息存入rxHeader,具体数据存入buf
    if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &rxHeader, buf) != HAL_OK) {
        //读取失败（这个情况是非常少见的，FIFO0被抢空了），返回空错误
        return CAN_ERR_EMPTY;
    }
    //判断该帧是标准帧(11位ID)还是扩展帧（29位ID）
    if (rxHeader.IDE == CAN_ID_STD) {
        //标准帧：把StdId返回给调用者
        *id = (uint16_t)rxHeader.StdId;
    } else {
        //扩展帧：把ExtId截断为16位返回给调用者，会丢失高位
        *id = (uint16_t)rxHeader.ExtId;
    }
    //把数据长度返回给调用者
    *len = (uint8_t)rxHeader.DLC;
    //读取成功则返回成功状态
    return CAN_OK;
}

/**
 * @brief  配置 CAN1 滤波器为"全部放行"（调试用，接收所有ID）
 * @note   CAN1是主控制器，滤波器组从 bank0 开始，
 *         CAN2必须通过 SlaveStartFilterBank 告诉 HAL 分界点。
 *         32位掩码模式下，掩码全0表示不关心ID任何位 => 全部接收。
 */
void CAN1_Filter_AcceptAll(void)
{
    //定义滤波器配置结构体并清零（避免未初始化字段）
    CAN_FilterTypeDef canFilter = {0};
    //指定使用的滤波器组编号（0），CAN1从bank0起
    canFilter.FilterBank = CAN1_FILTER_BANK;
    //滤波模式：设置为掩码模式，另一种是ID列表模式
    canFilter.FilterMode = CAN_FILTERMODE_IDMASK;
    //滤波尺度：32位，另一种是16位
    canFilter.FilterScale = CAN_FILTERSCALE_32BIT;
    //ID高16位 = 0（CAN_FILTER_ALL定义为0x0000）
    canFilter.FilterIdHigh = CAN_FILTER_ALL;
    //ID低16位 = 0
    canFilter.FilterIdLow = CAN_FILTER_ALL;
    //掩码高16位 = 0
    canFilter.FilterMaskIdHigh = CAN_FILTER_ALL;
    //掩码低16位 = 0
    canFilter.FilterMaskIdLow = CAN_FILTER_ALL;
    //将匹配到的报文放入到FIFO0
    canFilter.FilterFIFOAssignment = CAN_RX_FIFO0;
    //使能改滤波器
    canFilter.FilterActivation = ENABLE;
    //从控制器CAN2的滤波器起始为14
    canFilter.SlaveStartFilterBank = CAN_SLAVE_START_BANK;
    //将以上配置写入CAN1滤波器
    if (HAL_CAN_ConfigFilter(&hcan1, &canFilter) != HAL_OK) {
        //失败则打印错误提示信息
        printf("CAN1 filter FAIL\r\n");
    }
}

/**
 * @brief  配置并使能 CAN1 接收中断
 * @note   使能 FIFO0（First In First Out，先进先出）消息挂起（Pending）中断，
 *         并在 NVIC（Nested Vectored Interrupt Controller，嵌套向量中断控制器）中设置优先级并开启。
 *         告诉硬件"如果 FIFO0 邮箱里收到新消息了，记得敲门（触发中断）叫我"，并接通门铃。
 */
void CAN1_IRQ_Enable(void)
{
    //激活CAN1的接收FIFO挂起中断
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        //激活失败则打印错误信息
        printf("CAN1 irq FAIL\r\n");
        //直接返回
        return;
    }
    //设置 CAN1 接收中断的优先级为 2（数字越小优先级越高），抢占优先级为 0。
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 2, 0);
    // 在 NVIC（Nested Vectored Interrupt Controller，嵌套向量中断控制器）中使能 CAN1 接收中断。
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
}

/**
 * @brief  CAN1 接收中断服务函数
 * @note   当 CAN1 发生中断时，CPU（Central Processing Unit，中央处理器）自动跳转到这里。
 *         直接调用 HAL（Hardware Abstraction Layer，硬件抽象层）库的通用中断处理函数，
 *         由 HAL 库自动判断中断源、清除标志位，并调用相应的回调函数。
 */
void CAN1_RX0_IRQHandler(void)
{
    HAL_CAN_IRQHandler(&hcan1);
}

/**
 * @brief  打印 CAN1 的错误状态（TEC/REC/LEC/BOFF 及 HAL 错误码与状态）
 * @note   直接读 ESR 寄存器（Error Status Register），提取：
 *         - TEC：发送错误计数器（Transmit Error Counter，bit7:0）
 *         - REC：接收错误计数器（Receive Error Counter，bit23:16）
 *         - LEC：最后错误代码（Last Error Code，bit6:4）
 *         - BOFF：Bus-Off 标志（bit2）
 *         同时调用 HAL 接口获取累计错误码与当前状态机状态。
 */
void CAN1_PrintStatus(void)
{
    uint32_t esr = hcan1.Instance->ESR;

    // 打印一行：TEC、REC、LEC、BOFF，以及 HAL 错误码和状态
    printf("TEC=%lu REC=%lu LEC=%lu BOFF=%lu | ERR=0x%08lX STATE=%d\r\n",
    // ESR[7:0]  = TEC 发送错误计数器（0~255）
    (unsigned long) (esr & 0xFFu),
    // ESR[23:16] = REC 接收错误计数器（0~255）
    (unsigned long) ((esr >> 16) & 0xFFu),
    // ESR[6:4]  = LEC 最后错误代码（0~7，3位）
    (unsigned long) ((esr >> 4) & 0x7u),
    // ESR[2]    = BOFF Bus-Off 标志（1 表示已进入 Bus-Off）
    (unsigned long) ((esr >> 2) & 0x1u),
    // HAL_CAN_GetError() 返回累计的 HAL 错误标志（位掩码）
    (unsigned long)HAL_CAN_GetError(&hcan1),
    // HAL_CAN_GetState() 返回 CAN 状态机状态（READY/BUSY/ERROR 等）
    (int)HAL_CAN_GetState(&hcan1));
}
