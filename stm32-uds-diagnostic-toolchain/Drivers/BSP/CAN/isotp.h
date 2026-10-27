/**
 ****************************************************************************************************
 * @file        isotp.h
 * @author      <aloneWalker>
 * @version     V1.0
 * @date        2026-10-27
 * @brief       ISO 15765-2 传输层（ECU 侧）
 ****************************************************************************************************
 * @attention
 *
 * 帧类型由首字节高 4 位决定：0=SF 1=FF 2=CF 3=FC
 * ECU 侧回长报文时 BS=0（不限块）、STmin=5ms
 *
 ****************************************************************************************************
 */

#ifndef __CAN_ISOTP_H
#define __CAN_ISOTP_H

#include "./SYSTEM/sys/sys.h"

/* ------------------------- PCI 帧类型 ------------------------------- */
//高 4 位：帧类型
#define ISOTP_PCI_MASK          0xF0u
//单帧
#define ISOTP_PCI_SF            0x00u
//首帧
#define ISOTP_PCI_FF            0x10u
//连续帧
#define ISOTP_PCI_CF            0x20u
//流控帧
#define ISOTP_PCI_FC            0x30u

/* ------------------------ 长度与序号 -------------------------------- */
//低 4 位：SF 长度 / CF 序号
#define ISOTP_LEN_MASK          0x0Fu
//单帧最多 7 字节数据
#define ISOTP_SF_MAX            7
//首帧带 6 字节数据
#define ISOTP_FF_DATA_LEN       6
//连续帧带 7 字节数据
#define ISOTP_CF_DATA_LEN       7
//连续帧序号 0~15 回绕
#define ISOTP_SN_MASK           0x0Fu
//首帧之后的第一个连续帧序号
#define ISOTP_SN_FIRST          1

/* -------------------------- 流控帧 -------------------------------- */
//继续发送
#define ISOTP_FC_CTS            0x00u
//等待
#define ISOTP_FC_WAIT           0x01u
//溢出
#define ISOTP_FC_OVFLW          0x02u

/* --------------------------- 其它 ---------------------------------- */
//填充字节（接收方必须忽略）
#define ISOTP_PAD_BYTE          0xCCu
#define ISOTP_OK                0
#define ISOTP_ERR               1

/* 收发缓冲（中断与主循环共用，ISR 只置标志） */
extern uint8_t g_iso_rx_buf[];
extern uint16_t g_iso_rx_len;
//1 = 有一包完整请求待处理
extern volatile uint8_t g_iso_req_pending;

/* 函数声明 */
//初始化收发 ID 与状态机
void isotp_init(uint16_t rx_id, uint16_t tx_id);
//收一帧 CAN 数据（由 CAN 中断调用）
void isotp_rx_frame(uint16_t id, uint8_t *data, uint8_t len);
//发一包数据，阻塞式，含 FC 等待
uint8_t isotp_send(uint8_t *buf, uint16_t len);
//是否正在发多帧
uint8_t isotp_is_busy(void);
//清空收发状态
void isotp_reset(void);

#endif
