#ifndef __CAN_COMMON_H
#define __CAN_COMMON_H

#include "./SYSTEM/sys/sys.h"
#include "stdio.h"

/* -------------------------- 波特率选择 ------------------------------ */
// 波特率选择值：500kbps
#define CAN_500K                0
// 波特率选择值：250kbps
#define CAN_250K                1
// 波特率选择值：125kbps
#define CAN_125K                2

/* 位时序：APB1=42MHz 时，一个位 = 84 个时钟 = 14tq x BRP */
// 500kbps 时的预分频值（84/14=6）
#define CAN_PRESCALER_500K      6
// 250kbps 时的预分频值（翻倍）
#define CAN_PRESCALER_250K      12
// 125kbps 时的预分频值（再翻倍）
#define CAN_PRESCALER_125K      24
// 时间段1为11个tq
#define CAN_BS1_TQ              CAN_BS1_11TQ
// 时间段2为2个tq
#define CAN_BS2_TQ              CAN_BS2_2TQ
// 同步跳转宽度为1个tq
#define CAN_SJW_TQ              CAN_SJW_1TQ

/* 滤波器：CAN1 占 bank 0~13，CAN2 从 bank 14 起 */
// CAN2 使用的滤波器组编号（从bank14开始
#define CAN_FILTER_BANK         14
// CAN2（从控制器）滤波器起始bank编号
#define CAN_SLAVE_START_BANK    14
// 滤波器掩码/ID全0（配合掩码模式可实现全放行）
#define CAN_FILTER_ALL          0x0000
// 标准帧ID掩码（11位，最大0x7FF）
#define CAN_STD_ID_MASK         0x7FFu

/* -------------------------- 收发返回值 ------------------------------ */
// 操作成功返回值
#define CAN_OK                  0
// 参数错误返回值
#define CAN_ERR_PARAM           1
// 无可用发送邮箱返回值
#define CAN_ERR_NOBOX           2
// 接收FIFO为空返回值
#define CAN_ERR_EMPTY           3

/* 一帧最大数据长度 */
// CAN帧数据段最大长度（经典CAN为8字节）
#define CAN_DLC_MAX             8

#endif
