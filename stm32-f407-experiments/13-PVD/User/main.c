#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/PWR/pwr.h"


int main(void)
{ 
    
    uint8_t t = 0;
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                         /* 初始化LED */
    lcd_init();                         /* 初始化LCD */
    pwr_pvd_init(PWR_PVDLEVEL_7);       /* 初始化PVD */
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "PVD TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    
    lcd_show_string(30, 130, 200, 16, 16, "PVD Voltage OK! ", BLUE);
    
    
    
    
    while (1)
    {
        
       if (++t == 20)
        {
            t = 0;
            LED0_TOGGLE();
        }
        
        delay_ms(10);
        
        
        
    }
}
