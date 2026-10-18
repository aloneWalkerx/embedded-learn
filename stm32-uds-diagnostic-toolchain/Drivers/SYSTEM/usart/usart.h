#ifndef __CAN_USART_H
#define __CAN_USART_H

#include "./SYSTEM/sys/sys.h"
#include "stdio.h"
//1，发送引脚定义
//发送引脚对应的引脚类型
#define CAN_USARTX_TX_GPIO_PORT                         GPIOA
//发送引脚对应的引脚号
#define CAN_USARTX_TX_GPIO_PIN                          GPIO_PIN_9
//使能发送引脚
#define CAN_USARTX_TX_GPIO_CLK_ENABLE()                 do{__HAL_RCC_GPIOA_CLK_ENABLE();}while(0)
//发送引脚对应的复用号
#define CAN_USARTX_TX_GPIO_AF                           GPIO_AF7_USART1

//2，接收引脚定义
//接收引脚对应的引脚类型
#define CAN_USARTX_RX_GPIO_PORT                         GPIOA
//接受引脚对应的引脚号
#define CAN_USARTX_RX_GPIO_PIN                          GPIO_PIN_10
//使能接收引脚
#define CAN_USARTX_RX_GPIO_CLK_ENABLE()                 do{__HAL_RCC_GPIOA_CLK_ENABLE();}while(0)
//接收引脚对应的复用号
#define CAN_USARTX_RX_GPIO_AF                           GPIO_AF7_USART1

//3，具体的USART
//使用USART1
#define CAN_USARTX                                      USART1
//USART1对应的中断
#define CAN_USARTX_IRQn                                 USART1_IRQn
//USART1对应的中断服务函数
#define CAN_USARTX_IRQHandler                           USART1_IRQHandler
//使能USART1
#define CAN_USARTX_CLK_ENABLE()                         do{__HAL_RCC_USART1_CLK_ENABLE();}while(0)

//4,定义接收缓冲区大小
//每次接收一个字节
#define CAN_USARTX_RX_BUFFER_SIZE                        1
//最大接收520字节
#define CAN_USARTX_RX_BUFFER_MAX                         520

//5，定义全局变量
//UART句柄
extern UART_HandleTypeDef  g_uart_handle;
//定义一个接收数据以及数据状态的变量
extern uint16_t g_can_usartx_status_data;
//定义单次接收缓冲数组变量
extern uint8_t  g_can_usartx_rx_buffer[CAN_USARTX_RX_BUFFER_SIZE];
//定义最大接收缓冲数组变量
extern uint8_t  g_can_usartx_rx_buffer_max[CAN_USARTX_RX_BUFFER_MAX];

//6，函数声明
//USART初始化函数
void can_usartx_init(uint32_t baudrate);


#endif 
