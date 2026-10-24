#include "./BSP/CAN/can_test.h"
#include "./BSP/CAN/can.h"
#include "./SYSTEM/delay/delay.h"
#include "stdio.h"

/**
 * @brief  CAN2 环回自测：发送 N 帧，再收回并校验，统计成功次数
 * @return 成功完成"发-收-校验"的帧数（0 ~ TEST_LOOP_CNT）
 * @note   需在回环模式下调用（CAN2_Init 的 loopback 参数传 1），
 *         发送的帧会被自己收到，用于验证收发链路是否正常
 */
void can_test_loopback(void)
{
    // 存放接收到的帧ID
    uint16_t id;
    // 接收数据缓冲区（8字节）
    uint8_t buf[CAN_DLC_MAX];
    // 存放接收到的数据长度（DLC）
    uint8_t len;
    // 循环变量：遍历数据字节
    uint8_t i;
    // 成功计数：发-收-校验全部通过的帧数
    uint16_t ok = 0;
    // 循环变量：测试帧序号
    uint16_t k;

    printf("\r\n---- 环回自测 ----\r\n");

    //初始化CAN
    CAN2_Init(CAN_500K, 1);
    // 配置滤波器为全部放行（环回时也要过滤波器才能收到）
    CAN2_Filter_AcceptAll();
    // 循环发送 TEST_LOOP_CNT 帧
    for (k = 0; k < TEST_LOOP_CNT; k++) {
        // 遍历8个数据字节
        for (i = 0; i < CAN_DLC_MAX; i++) {
            // 填充测试数据：第k帧第i字节 = k+i（便于校验
            buf[i] = (uint8_t) (k + i);
        }
        // 发送一帧标准数据帧（ID=TEST_TX_ID，8字节）
        if (CAN2_SendStd(CAN2_TEST_TX_ID, buf, CAN_DLC_MAX) != CAN_OK) {
            // 发送失败打印序号
            printf("send fail at %u\r\n", k);
            // 本帧失败，跳过接收，进入下一帧
            continue;
        }
        // 延时2ms，等回环数据进入FIFO（回环是瞬时的，但留余量更稳）
        delay_ms(2);
        // 尝试从FIFO0读一帧回来
        if (CAN2_RecvStd(&id, buf, &len) == CAN_OK) {
            // 校验：ID、长度、首字节，3项全对才算通过
            if ((id == CAN2_TEST_TX_ID) && (len == CAN_DLC_MAX) && (buf[0] == k)) {
                // 三项校验全通过，成功计数加1
                ok++;
            }
        }
        // 没收到数据（回环模式下不应发生），打印提示
        else {
            // 没收到数据（回环模式下不应发生），打印提示
            printf("recv empty at %u\r\n", k);
        }
    }
    // 打印成功数/总数
    printf("环回结果: %u/%u\r\n", ok, TEST_LOOP_CNT);
    // 返回成功帧数，供调用者判断测试是否通过
}

// 正常模式测试函数（通过中断接收数据）
void can_test_normal(void)
{
    // 定义一个 8 字节的数组，用于存放即将发送的数据
    uint8_t buf[8];
    // 循环变量，用于遍历数组
    uint8_t i;
    // 计数器，每次循环加 1，用于生成递增的测试数据
    uint16_t tick = 0;
    // 记录上一次接收到的报文总数，用于比对是否有新数据
    uint32_t last_cnt = 0;

    printf("\r\n---- 正常模式（中断收帧）----\r\n");

    // 初始化 CAN2 为 500K 波特率（Baud Rate，通信速率），参数 0 表示正常模式（非环回模式）
    //0 = 正常模式
    CAN2_Init(CAN_500K, 0);

    // 配置 CAN2 的接收过滤器，全部放行（接收总线上所有的 ID 报文）
    CAN2_Filter_AcceptAll();

    // 使能 CAN2 的 FIFO0 接收中断，并在 NVIC（Nested Vectored Interrupt Controller，嵌套向量中断控制器）中开启
    CAN2_IRQ_Enable();

    // 进入主循环，程序将一直在这里面运行
    while (1) {
        // 填充发送数据：每个字节的值 = 当前 tick 值 + 字节索引（0~7）
        for (i = 0; i < 8; i++) {
            buf[i] = (uint8_t) (tick + i);
        }

        // 调用发送函数，将 8 字节数据打包成标准数据帧发送出去，ID 为 TEST_TX_ID（通常是 0x123）
        CAN2_SendStd(CAN2_TEST_TX_ID, buf, 8);

        // 延时等待（TEST_PERIOD_MS 通常是 100ms），控制发送频率
        delay_ms(TEST_PERIOD_MS);

        // 发送完一帧后，tick 加 1，为下一轮生成不同的数据内容
        tick++;

        // 检查全局接收计数器 g_rx_cnt 是否发生变化（说明中断里收到了新数据）
        if (g_can2_rx_cnt != last_cnt) {            /* 主循环只轮询标志，不在中断里打印 */
            // 更新 last_cnt，以免重复打印同一条数据
            last_cnt = g_can2_rx_cnt;

            // 打印接收到的报文信息：ID（Identifier，标识符）、DLC（Data Length Code，数据长度）、首字节数据和总计接收数
            printf("RX id=0x%03X len=%u d0=%02X  total=%lu\r\n", g_can2_rx_id, g_can2_rx_len, g_can2_rx_buf[0],
            (unsigned long)g_can2_rx_cnt);
        }
    }
}

// 过滤指定ID
void can_filter_list2()
{
    // 定义一个 8 字节的数组，用于存放即将发送的数据
    uint8_t buf[8];
    // 循环变量，用于遍历数组
    uint8_t i;
    // 计数器，每次循环加 1，用于生成递增的测试数据
    uint16_t tick = 0;
    // 记录上一次接收到的报文总数，用于比对是否有新数据
    uint32_t last_cnt = 0;

    printf("\r\n---- 列表模式（中断收帧）----\r\n");

    // 初始化 CAN2 为 500K 波特率（Baud Rate，通信速率），参数 0 表示正常模式（非环回模式）
    //0 = 正常模式
    CAN2_Init(CAN_500K, 0);

    // 配置 CAN2 的接收过滤器，只放行指定的ID
    CAN2_Filter_List2(0x7E0, 0x7DF);

    // 使能 CAN2 的 FIFO0 接收中断，并在 NVIC（Nested Vectored Interrupt Controller，嵌套向量中断控制器）中开启
    CAN2_IRQ_Enable();

    // 进入主循环，程序将一直在这里面运行
    while (1) {
        // 填充发送数据：每个字节的值 = 当前 tick 值 + 字节索引（0~7）
        for (i = 0; i < 8; i++) {
            buf[i] = (uint8_t) (tick + i);
        }

        // 调用发送函数，将 8 字节数据打包成标准数据帧发送出去，ID 为 TEST_TX_ID（通常是 0x123）
        CAN2_SendStd(CAN2_TEST_TX_ID, buf, 8);

        // 延时等待（TEST_PERIOD_MS 通常是 100ms），控制发送频率
        delay_ms(TEST_PERIOD_MS);

        // 发送完一帧后，tick 加 1，为下一轮生成不同的数据内容
        tick++;

        // 检查全局接收计数器 g_rx_cnt 是否发生变化（说明中断里收到了新数据）
        if (g_can2_rx_cnt != last_cnt) {            /* 主循环只轮询标志，不在中断里打印 */
            // 更新 last_cnt，以免重复打印同一条数据
            last_cnt = g_can2_rx_cnt;

            // 打印接收到的报文信息：ID（Identifier，标识符）、DLC（Data Length Code，数据长度）、首字节数据和总计接收数
            printf("RX id=0x%03X len=%u d0=%02X  total=%lu\r\n", g_can2_rx_id, g_can2_rx_len, g_can2_rx_buf[0],
            (unsigned long)g_can2_rx_cnt);
        }
    }
}

/* ============================================================
 * 第二关：三节点自检（CAN1 ? CAN2 互发，分析仪旁听）
 * ============================================================ */
void can_node_test(void)
{
    uint8_t buf[8];
    uint8_t i;
    uint32_t tick = 0;
    uint32_t last1 = 0, last2 = 0;
    uint32_t esr1, esr2;

    printf("\r\n---- 三节点自检（CAN1 -> 0x%03X，CAN2 -> 0x%03X）----\r\n",
    (unsigned)CAN1_TEST_TX_ID, (unsigned)CAN2_TEST_TX_ID);

    CAN2_Init(CAN_500K, 0);                         /* 节点1：正常模式 */
    CAN1_Init(CAN_500K, 0);                         /* 节点2：正常模式 */
    CAN2_Filter_AcceptAll();
    CAN1_Filter_AcceptAll();
    CAN2_IRQ_Enable();
    CAN1_IRQ_Enable();

    while (1) {
        /* 节点2 发 → 节点1 和分析仪收 */
        for (i = 0; i < 8; i++) {
            buf[i] = (uint8_t) (tick + i);
        }
        CAN1_SendStd(CAN1_TEST_TX_ID, buf, 8);
        delay_ms(200);

        /* 节点1 发 → 节点2 和分析仪收 */
        for (i = 0; i < 8; i++) {
            buf[i] = (uint8_t) (tick + 0x80 + i);
        }
        CAN2_SendStd(CAN2_TEST_TX_ID, buf, 8);
        delay_ms(200);

        tick++;

        if (last2 != g_can2_rx_cnt) {
            last2 = g_can2_rx_cnt;
            printf("CAN2 RX id=0x%03X d0=%02X cnt=%lu\r\n",
            (unsigned)g_can2_rx_id, (unsigned)g_can2_rx_buf[0],
            (unsigned long)g_can2_rx_cnt);
        }
        if (last1 != g_can1_rx_cnt) {
            last1 = g_can1_rx_cnt;
            printf("CAN1 RX id=0x%03X d0=%02X cnt=%lu\r\n",
            (unsigned)g_can1_rx_id, (unsigned)g_can1_rx_buf[0],
            (unsigned long)g_can1_rx_cnt);
        }
        if ((tick % 5u) == 0u) {
            esr1 = hcan1.Instance->ESR;
            esr2 = hcan2.Instance->ESR;
            printf("[%lu] CAN1 TEC=%lu REC=%lu | CAN2 TEC=%lu REC=%lu | rx1=%lu rx2=%lu\r\n",
            (unsigned long)tick,
            (unsigned long) (esr1 & 0xFFu), (unsigned long) ((esr1 >> 16) & 0xFFu),
            (unsigned long) (esr2 & 0xFFu), (unsigned long) ((esr2 >> 16) & 0xFFu),
            (unsigned long)g_can1_rx_cnt, (unsigned long)g_can2_rx_cnt);
        }
    }
}
