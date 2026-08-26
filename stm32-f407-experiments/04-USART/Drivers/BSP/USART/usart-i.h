#ifndef __USART_I_H
#define __USART_I_H

#include "stdio.h"
#include "./SYSTEM/sys/sys.h"


/*
    串口以及引脚定义：
    此处以通用形式定义，便于与后续切换其他引脚以及串口，初始是USART1
*/

//1，定义USART1的发送引脚（根据芯片手册得知USART1的引脚类型为GPIOA且发送引脚为PA9）
#define  USART_I_TX_GPIO_PORT                           GPIOA

//定义USART1的发送引脚号
#define  USART_I_TX_GPIO_PIN                            GPIO_PIN_9

//定义USART1发送引脚的复用号
#define  USART_I_TX_GPIO_AF                             GPIO_AF7_USART1

//使能发送引脚
#define  USART_I_TX_GPIO_CLK_ENABLE()                   do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)


//2，定义USART1的接收引脚（根据芯片手册得知USART1的引脚类型为GPIOA且接收引脚为PA10）
#define  USART_I_RX_GPIO_PORT                           GPIOA

//定义USART1的发送引脚号
#define  USART_I_RX_GPIO_PIN                            GPIO_PIN_10

//定义USART1发送引脚的复用号
#define  USART_I_RX_GPIO_AF                             GPIO_AF7_USART1

//使能发送引脚
#define  USART_I_RX_GPIO_CLK_ENABLE()                   do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)


//3，定义串口
//设置串口为USART1
#define  USART_I_UX                                     USART1

//定义串口中断
#define  USART_I_UX_IRQn                                USART1_IRQn

//定义USART1的中断服务函数
#define  USART_I_UX_IRQHandler                          USART1_IRQHandler    

//使能USART1串口
#define  USART_I_UX_CLK_ENABLE()                          do { __HAL_RCC_USART1_CLK_ENABLE(); } while (0)


//定义最大接收字节
#define  USART_I_REC_LEN   200

//定义串口使能状态，默认为1,1 ：接收，0 ：禁止、
#define  USART_I_ENABLE_STATUS       1

//定义缓存大小
#define  I_RXBUFFERSIZE       1

//定义UART实例(多个串口多个实例)
extern  UART_HandleTypeDef      g_uart_i_handle;

//定义接收缓冲,最大USART_REC_LEN个字节.末字节为换行符
extern  uint8_t     g_usart_i_rx_buff_data[USART_I_REC_LEN];

//定义接收状态标记以及接收的数据
extern  uint16_t    g_usart_i_rx_status_data;

//HAL库USART接收Buffer
extern  uint8_t     g_usart_i_rx_buffer[I_RXBUFFERSIZE];

//串口初始化函数(baudrate：波特率)
void  usart_i_init(uint32_t  baudrate);



#endif

