#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/KEY/key-i.h"

int main(void)
{   
    uint8_t key;
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                         /* 初始化LED */
    key_i_init();                         /* 初始化按键 */
    while (1)
    {
        key = key_i_scan(0);              /* 扫描按键 */
        
        switch (key)
        {
            case WKUP_PRESS:             /* WKUP按键被按下 */
            {
                LED0_TOGGLE();          /* 翻转LED0状态 */
                break;
            }
            case KEY0_PRESS:             /* KEY0按键被按下 */
            {
                LED1_TOGGLE();          /* 翻转LED1状态 */
                break;
            }
        }
        
        delay_ms(10);
    }
}
