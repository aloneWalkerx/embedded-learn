#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/KEY/key-i.h"
#include "./BSP/KEY/key-e.h"
#include "./BSP/LED/led-e.h"
int main(void)
{   
    uint8_t key;
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                         /* 初始化板载LED */
    led_e_init();                         /* 初始化板载LED */
    key_i_init();                         /* 初始化板载按键 */
    key_e_init();                         /* 初始化外设按键 */
    while (1)
    {
        
       
        //外设按键翻转外设LED       
        key = key_e_scan(0);              /* 扫描外设按键 */
        
        switch (key)
        {
            case KEY_R_PRESS:             /* 外设KEY_R按键被按下 */
            {
                LED_R_TOGGLE();          /* 翻转外设LED_R状态 */
                break;
            }
            case KEY_Y_PRESS:             /* 外设KEY_Y按键被按下 */
            {
                LED_Y_TOGGLE();          /* 翻转外设LED_Y状态 */
                break;
            }
            case KEY_G_PRESS:             /* 外设KEY_G按键被按下 */
            {
                LED_G_TOGGLE();          /* 翻转外设LED_G状态 */
                break;
            }
        }
        
        
           //板载按键翻转板载LED
//        key = key_i_scan(0);              /* 扫描板载按键 */
//        
//        switch (key)
//        {
//            case WKUP_PRESS:             /* 板载WKUP按键被按下 */
//            {
//                LED0_TOGGLE();          /* 翻转板载LED0状态 */
//                break;
//            }
//            case KEY0_PRESS:             /* 板载KEY0按键被按下 */
//            {
//                LED1_TOGGLE();          /* 翻转板载LED1状态 */
//                break;
//            }
//        }
//        
        delay_ms(10);
    }
}
