#ifndef __CAN_TEST_H
#define __CAN_TEST_H

#include "./SYSTEM/sys/sys.h"
//环回自测帧数
#define TEST_LOOP_CNT       100
//自发自收用的 ID
//#define CAN2_TEST_TX_ID     0x123
//双节点时对端发的 ID
#define TEST_TX_ID_PEER     0x100
//发送周期
#define TEST_PERIOD_MS      100
//环回自测帧数
#define TEST_LOOP_CNT       100
//ECU 收请求（PC -> ECU）
#define ISO_RX_ID           0x7E0u

/* -------------------------- 变量定义 ------------------------------ */
//接收到的id
extern volatile uint16_t g_can2_rx_id;
//接收数据的长度
extern volatile uint8_t g_can2_rx_len;
//接收数据缓冲
extern volatile uint8_t g_can2_rx_buf[8];
//接收总条数
extern volatile uint32_t g_can2_rx_cnt;

//环回测试
void can_test_loopback(void);
//正常模式
void can_test_normal(void);
//过滤指定ID
void can_filter_list2(void);

void can_node_test(void);

extern volatile uint16_t g_can1_rx_id;              /* CAN1 收到的 */
extern volatile uint8_t g_can1_rx_len;
extern volatile uint8_t g_can1_rx_buf[8];
extern volatile uint32_t g_can1_rx_cnt;

#define TEST_MODE_2NODE     1                       /* 1=三节点自检  0=单节点正常模式 */
#define TEST_TX_ID          0x100                   /* 单节点模式：本板发出的 ID */
#define TEST_PERIOD_MS      100                     /* 单节点模式：发送周期 */
#define CAN1_TEST_TX_ID     0x111                   /* 三节点：CAN1 发的 ID（CAN2/分析仪应收到） */
#define CAN2_TEST_TX_ID     0x112                   /* 三节点：CAN2 发的 ID（CAN1/分析仪应收到） */

#endif
