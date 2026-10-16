#ifndef __USART_ME_H
#define __USART_ME_H
#include "./SYSTEM/SYS/sys.h"
#include "stdio.h"

#define USARTX_TX_GPIO_PIN_PORT         GPIOA
#define USARTX_TX_GPIO_PIN              GPIO_PIN_9
#define USARTX_TX_GPIO_CLK_ENANBLE()    do{__HAL_RCC_GPIOA_CLK_ENABLE();}while(0)
#define USARTX_TX_GPIO_AF               GPIO_AF7_USART1

#define USARTX_RX_GPIO_PIN_PORT         GPIOA
#define USARTX_RX_GPIO_PIN              GPIO_PIN_10
#define USARTX_RX_GPIO_CLK_ENABLE()     do{__HAL_RCC_GPIOA_CLK_ENABLE();}while(0)
#define USARTX_RX_GPIO_AF               GPIO_AF7_USART1

#define USARTX                          USART1
#define USARTX_IRQn                     USART1_IRQn
#define USARTX_IRQHandler               USART1_IRQHandler
#define USARTX_CLK_ENABLE()             do{__HAL_RCC_USART1_CLK_ENABLE();}while(0)

#define USARTX_RX_BUFFER_SIZE            1
#define USARTX_RX_BUFFER_SIZE_MAX        200

extern UART_HandleTypeDef g_usartx_handle;
extern uint8_t g_usartx_rx_buffer[USARTX_RX_BUFFER_SIZE];
extern uint8_t g_usartx_rx_buffer_max[USARTX_RX_BUFFER_SIZE_MAX];
extern uint16_t g_usartx_rx_status_data;

void usart_init_me(uint32_t baudRate);

#endif
