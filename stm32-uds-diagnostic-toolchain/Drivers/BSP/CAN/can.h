/**
 ****************************************************************************************************
 * @file        can.h
 * @author      <aloneWalker>
 * @version     V1.0
 * @date        2026-10-21
 * @brief       CAN2 驱动（bxCAN + HAL）
 *
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 M144Z-M4最小系统板STM32F407
 * 引脚    :PB12 = CAN2_RX(AF9) / PB13 = CAN2_TX(AF9)
 * 位时序  :APB1=42MHz，500kbps => 84 = (1+11+2) x 6，采样点 85.7%
 * 中断    :CAN2 有独立向量 CAN2_RX0_IRQn，不是 CAN1_RX0_IRQn
 *
 ****************************************************************************************************
 */

#ifndef __CAN_H
#define __CAN_H

#include "./SYSTEM/sys/sys.h"

/* --------------------------- 引脚定义 ------------------------------- */
// CAN2 所用GPIO端口为GPIOB
#define CAN_GPIO_PORT                   GPIOB
// CAN2 接收引脚为PB12
#define CAN_GPIO_RX_PIN                 GPIO_PIN_12
// CAN2 发送引脚为PB13
#define CAN_GPIO_TX_PIN                 GPIO_PIN_13
// 引脚复用功能选择AF9（对应CAN2）
#define CAN_GPIO_AF                     GPIO_AF9_CAN2
//具体对应的CAN
#define CAN_CANX                        CAN2

/* --------------------------- 使能时钟 ------------------------------- */
//使能GPIOB时钟
#define CAN_GPIO_PORT_CLK_ENABLE()      do { __HAL_RCC_GPIOB_CLK_ENABLE();}while(0)
//使能CAN1
#define CAN_CAN1_CLK_ENABLE()           do { __HAL_RCC_CAN1_CLK_ENABLE();}while(0)
//使能CAN2
#define CAN_CAN2_CLK_ENABLE()           do { __HAL_RCC_CAN2_CLK_ENABLE();}while(0)

/*------------------------------中断----------------------------------*/
#define CAN_CAN2_RX_IRQn                CAN2_RX0_IRQn
#define CAN_CAN2_RX_IRQHandler          CAN2_RX0_IRQHandler

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

/* CAN2 句柄 */
// 声明CAN2的HAL句柄（在can.c中定义）
extern CAN_HandleTypeDef hcan2;

/* 函数声明 */
//初始化 CAN2（时钟、引脚、位时序）
void CAN2_Init(uint8_t baud_sel, uint8_t loopback);
//发送一帧标准数据帧
uint8_t CAN2_SendStd(uint16_t id, uint8_t *buf, uint8_t len);
//从 FIFO0 读一帧（轮询）
uint8_t CAN2_RecvStd(uint16_t *id, uint8_t *buf, uint8_t *len);
//滤波器：全部放行（调试用）
void CAN2_Filter_AcceptAll(void);
//打开 FIFO0 接收中断
void CAN2_IRQ_Enable(void);

#endif
