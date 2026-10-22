#ifndef __CAN_TEST_H
#define __CAN_TEST_H

#include "./SYSTEM/sys/sys.h"
//环回自测帧数
#define TEST_LOOP_CNT       100
//自发自收用的 ID
#define TEST_TX_ID          0x123
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
extern volatile uint16_t g_rx_id;
//接收数据的长度
extern volatile uint8_t g_rx_len;
//接收数据缓冲
extern volatile uint8_t g_rx_buf[8];
//接收总条数
extern volatile uint32_t g_rx_cnt;

//环回测试
void can_test_loopback(void);
//正常模式
void can_test_normal(void);

#endif
