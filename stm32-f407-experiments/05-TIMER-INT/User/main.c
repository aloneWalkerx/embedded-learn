#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/TIMER/b-timer.h"

int main(void)
{ 
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                       /* 初始化LED */
    //(PSC + 1) × (ARR + 1) = 84,000,000 / 目标频率
    btim_timx_int_init(5000 - 1, 8400 - 1); /* 初始化基本定时器，溢出频率为2Hz */
    while (1)
    {
        
        LED0_TOGGLE();
        delay_ms(200);
        
        
    }
}
