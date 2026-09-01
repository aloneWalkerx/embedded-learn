#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/LCD/lcd.h"
#include "./USMART/usmart.h"

/**
 * @brief   LED状态设置
 * @param   无
 * @retval  无
 */
void led_set(uint8_t sta)
{
    LED1(sta);
}

/**
 * @brief   测试函数参数调用
 * @param   无
 * @retval  无
 */
void test_fun(void (*ledset)(uint8_t), uint8_t sta)
{
    ledset(sta);
}


int main(void)
{ 
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                         /* 初始化LED */
    lcd_init();                         /* 初始化LCD */
    usmart_dev.init(84);                /* 初始化USMART */
    
    
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "USMART TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    while (1)
    {
     LED0_TOGGLE();
     delay_ms(500);   
      
        
        
        
    }
}
