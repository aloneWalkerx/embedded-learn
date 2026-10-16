#include "./SYSTEM/usart/usart-me.h"

#pragma import(__use_no_semihosting)

struct __FILE {
    int handle;
};

int _ttywrch(int ch) {
    ch = ch;
    return ch;
}

void _sys_exit(int x) {

    x = x;
}

char *_sys_command_string(char *cmd, int len) {

    return NULL;
}

FILE __stdout;

int fputc(int ch, FILE *f) {
    while ((USART1->SR &0X40) == 0);
    USART1->DR = (uint8_t)ch;
    return ch;
}

UART_HandleTypeDef g_usartx_handle;
uint8_t g_usartx_rx_buffer[USARTX_RX_BUFFER_SIZE];
uint8_t g_usartx_rx_buffer_max[USARTX_RX_BUFFER_SIZE_MAX];
uint16_t g_usartx_rx_status_data = 0;

void usart_init_me(uint32_t baudRate) {
    g_usartx_handle.Instance = USARTX;
    g_usartx_handle.Init.BaudRate = baudRate;
    g_usartx_handle.Init.WordLength = UART_WORDLENGTH_8B;
    g_usartx_handle.Init.StopBits = UART_STOPBITS_1;
    g_usartx_handle.Init.Parity = UART_PARITY_NONE;
    g_usartx_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_usartx_handle.Init.Mode = UART_MODE_TX_RX;

    HAL_UART_Init(&g_usartx_handle);
    HAL_UART_Receive_IT(&g_usartx_handle, g_usartx_rx_buffer, USARTX_RX_BUFFER_SIZE);

}

void HAL_UART_MspInit(UART_HandleTypeDef *uart) {

    GPIO_InitTypeDef gpio_init_struct = {0};
    if (uart->Instance == USARTX) {
        USARTX_TX_GPIO_CLK_ENANBLE();
        USARTX_RX_GPIO_CLK_ENABLE();
        USARTX_CLK_ENABLE();
        gpio_init_struct.Pin = USARTX_TX_GPIO_PIN | USARTX_RX_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Alternate = USARTX_TX_GPIO_AF;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;

        HAL_GPIO_Init(USARTX_TX_GPIO_PIN_PORT, &gpio_init_struct);
        HAL_NVIC_SetPriority(USARTX_IRQn, 3, 3);
        HAL_NVIC_EnableIRQ(USARTX_IRQn);

    }

}
