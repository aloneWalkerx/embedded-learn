
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led-i.h"
#include  "./BSP/LED/led-e.h"
#include "./BSP/EXTI/exti-i.h"
#include  "./BSP/EXTI/exti-e.h"
#include  "./SYSTEM/sys/sys.h"






/**
HAL库外部中断回调函数
@param  GPIO_Pin : 外部中断线对应的引脚

*/
void HAL_GPIO_EXTI_Callback(uint16_t  GPIO_Pin)
{

    delay_ms(20);//机械按键消抖（仅演示，切勿在实际工程的中断服务函数种进行阻塞延时）
    switch (GPIO_Pin)
    {
        
         case WKUP_EXT_GPIO_PIN: /* WKUP按键对应引脚发生中断 */
        {
            LED0_TOGGLE();      /* 翻转LED0状态 */
            break;
        }
        case KEY0_EXT_GPIO_PIN: /* KEY0按键对应引脚发生中断 */
        {
            LED1_TOGGLE();      /* 翻转LED1状态 */
            break;
        }
        
        case KEY_R_EXT_GPIO_PIN: //KEY_R按键按下对应引脚发生中断
        {
             LED_R_TOGGLE();     //翻转LED_R状态
             break;
        }
        
        case KEY_Y_EXT_GPIO_PIN:  //KEY_R按键按下对应引脚发生中断
        {
             LED_Y_TOGGLE();      //翻转LED_Y状态
             break;
        }
        
        case KEY_G_EXT_GPIO_PIN:  //KEY_R按键按下对应引脚发生中断
        {
             LED_G_TOGGLE();      //翻转LED_G状态
             break;
        }
    
    }
}

