// 头文件保护，防止重复包含
#ifndef __USART_H
#define __USART_H

// 包含系统头文件（STM32 HAL库、GPIO定义等）
#include "./SYSTEM/sys/sys.h"
#include "stdio.h"

/* USART TX 引脚配置 */
#define USARTX_TX_GPIO_PIN_PORT              GPIOA  // TX引脚端口：GPIOA
#define USARTX_TX_GPIO_PIN                   GPIO_PIN_9 // TX引脚号：PA9
#define USARTX_TX_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOA_CLK_ENABLE();}while(0) // 使能GPIOA时钟
#define USARTX_TX_GPIO_AF                    GPIO_AF7_USART1 // TX引脚复用功能：USART1

/* USART RX 引脚配置 */
#define USARTX_RX_GPIO_PIN_PORT              GPIOA  // RX引脚端口：GPIOA
#define USARTX_RX_GPIO_PIN                   GPIO_PIN_10 // RX引脚号：PA10
#define USARTX_RX_GPIO_CLK_ENABLE()          do{__HAL_RCC_GPIOA_CLK_ENABLE();}while(0) // 使能GPIOA时钟
#define USARTX_RX_AF                         GPIO_AF7_USART1 // RX引脚复用功能：USART1

/* USART 外设选择 */
#define USARTX                               USART1 // 使用USART1
#define USARTX_IRQn                          USART1_IRQn // USART1中断号
#define USARTX_IRQHandler                    USART1_IRQHandler // USART1中断服务函数名
#define USARTX_USART1_CLK_ENABLE()           do{__HAL_RCC_USART1_CLK_ENABLE();}while(0)

/* 接收缓冲区大小定义 */
#define USARTX_RX_BUFFER_SIZE           1           // 接收缓冲区大小（字节）
#define USARTX_RX_BUFFER_SIZE_MAX            200    // 接收缓冲区最大大小

/* 全局变量声明 */
extern UART_HandleTypeDef g_usartx_handle;          // UART句柄
extern uint16_t g_usartx_rx_status_data;            // USART状态数据
extern uint8_t g_usart_rx_buffer[USARTX_RX_BUFFER_SIZE]; // 接收缓冲区
extern uint8_t g_usartx_rx_buffer_max[USARTX_RX_BUFFER_SIZE_MAX]; // 最大接收缓冲区

/* 函数声明 */
void usart_init(uint32_t baudrate);                 // USART初始化函数，参数为波特率

#endif
