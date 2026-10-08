#include "./BSP/DMA/dma.h"

DMA_HandleTypeDef g_dma_handle = {0};

UART_HandleTypeDef g_uartx_handle;

void dma_init() {
    DMAX_CLK_ENABLE();
    g_dma_handle.Instance = DMAX_STREAMX;
    g_dma_handle.Init.Channel = DMA_CHANNEL_4;
    g_dma_handle.Init.Direction = DMA_MEMORY_TO_PERIPH;
    g_dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    g_dma_handle.Init.MemInc = DMA_MINC_DISABLE;
    g_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    g_dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    g_dma_handle.Init.Mode = DMA_NORMAL;
    g_dma_handle.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    g_dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    g_dma_handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL;
    g_dma_handle.Init.MemBurst = DMA_MBURST_SINGLE;
    g_dma_handle.Init.PeriphBurst = DMA_PBURST_SINGLE;

    HAL_DMA_Init(&g_dma_handle);

    __HAL_LINKDMA(&g_dma_handle, hdmatx, g_dma_handle);

    HAL_NVIC_SetPriority(DMAX_STREAMX_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMAX_STREAMX_IRQn);

}

void DMAX_STREAMX_IRQHandler(void) {

    HAL_DMA_IRQHandler(&g_dma_handle);

}
