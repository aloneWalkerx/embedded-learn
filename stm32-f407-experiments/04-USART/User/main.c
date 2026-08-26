#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
//#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/USART/usart-i.h"

int main(void)
{
    uint16_t len;
    uint16_t times = 0;
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_i_init(115200);                 /* 初始化串口 */
    led_i_init();                        /* 初始化板载LED */
     
    while (1)
    {
        //整条数据接收完成
         if (g_usart_i_rx_status_data & 0x8000)
        {
            len = g_usart_i_rx_status_data & 0x3FFF;
            printf("\r\n来自STM32的回显数据，您发送的消息为：\r\n");
            HAL_UART_Transmit(&g_uart_i_handle, (uint8_t *)g_usart_i_rx_buff_data, len, HAL_MAX_DELAY);
            printf("\r\n\r\n");
            g_usart_i_rx_status_data = 0;
        }
        //未接收到完整数据，隔一定时间打印相关信息
        else
        {
            if ((times % 5000) == 0)
            {
                printf("\r\nSTM32F407ZGT6 串口通信实验\r\n");
                printf("正点原子@ALIENTEK\r\n\r\n\r\n");
            }
            if ((times % 200) == 0)
            {
                printf("请输入数据，以回车键结束\r\n");
            }
            if ((times % 30) == 0)
            {
                LED0_TOGGLE();
            }
            times++;
            delay_ms(10);
        }
       
        
        
    }
}
