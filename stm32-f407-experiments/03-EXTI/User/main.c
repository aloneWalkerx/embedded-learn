#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/LED/led-e.h"
#include "./BSP/EXTI/exti-i.h"
#include "./BSP/EXTI/exti-e.h"

int main(void)
{
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                         /* 初始化板载LED */
    led_e_init();                         /* 初始化外设LED */
    exti_i_init();                        /* 初始化板载外部中断 */
    exti_e_init();                        /* 初始化外设外部中断 */
    while (1)
    {
        
      
      //此中断实验是用于LED切换，所以不需要在此用whie循环的方式直接中断触发
      
      
    }
}
