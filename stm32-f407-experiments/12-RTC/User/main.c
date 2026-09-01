#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/LCD/lcd.h"
#include "./USMART/usmart.h"
#include "./BSP/RTC/rtc.h"

int main(void)
{ 
    
    uint8_t t = 0;
    char tbuf[40];
    uint8_t hour, minute, second, ampm;
    uint8_t year, month, date, week;
    
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                         /* 初始化LED */
    lcd_init();                         /* 初始化LCD */
    usmart_dev.init(84);                /* 初始化USMART */
    rtc_init();                         /* 初始化RTC */
    
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "RTC TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    
    /* 设置RTC周期性唤醒中断 */
    rtc_set_wakeup(RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0);
    
    while (1)
    {
        
      
        if ((t % 10) == 0)
        {
           
            /* 获取RTC日期信息 */
            rtc_get_date(&year, &month, &date, &week);
            sprintf(tbuf, "Date:20%02d-%02d-%02d", year, month, date);
            lcd_show_string(30, 140, 210, 16, 16, tbuf, RED);
            
             /* 获取RTC时间信息 */
            rtc_get_time(&hour, &minute, &second, &ampm);
            sprintf(tbuf, "Time:%02d:%02d:%02d", hour, minute, second);
            lcd_show_string(30, 120, 210, 16, 16, tbuf, RED);
            sprintf(tbuf, "Week:%d", week);
            lcd_show_string(30, 160, 210, 16, 16, tbuf, RED);
        }
        
        if (++t == 20)
        {
            t = 0;
            LED0_TOGGLE();
        }
        
        delay_ms(10);
        
    }
}
