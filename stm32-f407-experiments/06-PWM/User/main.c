#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-e.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/TIMER/g-timer.h"
int main(void)
{ 
    
    uint16_t compare = 0;
    uint8_t counter = 0;
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                                 /* 初始化LED */
    //gtim_timx_int_init(5000 - 1, 8400 - 1); /* 初始化通用定时器中断，溢出频率为2Hz */
    gtim_timx_pwm_chy_init(500 - 1, 84 - 1);    /* 初始化通用定时器PWM，PWM频率为2KHz */
    
    /* 通用定时器句柄 */
extern TIM_HandleTypeDef g_timx_pwm_handle;

    while (1)
    {
       
        if (compare++ >= 400)
        {
            compare = 0;
        }
        __HAL_TIM_SET_COMPARE(&g_timx_pwm_handle, GTIM_TIMX_PWM_CHX, compare);
        
        if (counter++ == 20)
        {
            counter = 0;
            LED0_TOGGLE();
        }
        
        delay_ms(5); 
      
        
    }
}
