#ifndef __CAN1_H
#define __CAN1_H

#include "./SYSTEM/sys/sys.h"

//CAN引脚类型
#define CAN1_GPIO_PORT                 GPIOA
//CAN1接收复用引脚
#define CAN1_GPIO_RX_PIN               GPIO_PIN_11
//CAN1发送复用引脚
#define CAN1_GPIO_TX_PIN               GPIO_PIN_12
//CAN1复用号
#define CAN1_GPIO_AF                   GPIO_AF9_CAN1
//使能CAN1
#define CAN1_CLK_ENABLE()              do { __HAL_RCC_CAN1_CLK_ENABLE();}while(0)
//使能GPIOA时钟
#define CAN1_GPIO_PORT_CLK_ENABLE()    do { __HAL_RCC_GPIOA_CLK_ENABLE();}while(0)
//CAN1的滤波器编号
#define CAN1_FILTER_BANK        0

//CAN1的HAL句柄
extern CAN_HandleTypeDef hcan1;

//初始化 CAN1
void CAN1_Init(uint8_t baud_sel, uint8_t loopback);
//CAN1 发一帧
uint8_t CAN1_SendStd(uint16_t id, uint8_t *buf, uint8_t len);
//CAN1 收一帧
uint8_t CAN1_RecvStd(uint16_t *id, uint8_t *buf, uint8_t *len);
//CAN1 滤波器全放行
void CAN1_Filter_AcceptAll(void);
//CAN1 接收中断
void CAN1_IRQ_Enable(void);
//CAN1 的 TEC/REC
void CAN1_PrintStatus(void);

#endif
