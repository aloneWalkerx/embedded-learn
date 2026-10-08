#ifndef __DMA_H
#define __DMA_H

#include "SYSTEM/sys/sys.h"

#define DMAX_STREAMX                DMA2_Stream7
#define DMAX_CLK_ENABLE()           do{__HAL_RCC_DMA2_CLK_ENABLE();}while(0)
#define DMAX_STREAMX_IRQn           DMA2_Stream7_IRQn
#define DMAX_STREAMX_IRQHandler     DMA2_Stream7_IRQHandler

extern UART_HandleTypeDef g_uartx_handle;
void dma_init(void);

#endif
