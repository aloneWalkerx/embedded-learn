#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/LCD/lcd.h"
#include "./USMART/usmart.h"
#include "./BSP/RTC/rtc.h"
#include "./BSP/BUZZER/buzzer.h"


extern volatile uint8_t g_alarm_triggered;
extern volatile uint8_t g_buzzer_count;
extern volatile uint8_t g_buzzer_state;
const char *week_str[] = {"Everyday", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
int main(void)
{ 
    /* 蜂鸣器状态机 */
    static uint32_t buzzer_tick = 0;
    uint8_t t = 0;
    char tbuf[40];
    uint8_t hour, minute, second, ampm;
    uint8_t year, month, date, week;
    uint8_t alarm_hour,alarm_minute,alarm_second,alarm_week;
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                         /* 初始化LED */
    lcd_init();                         /* 初始化LCD */
    usmart_dev.init(84);                /* 初始化USMART */
    rtc_init();                         /* 初始化RTC */
    buzzer_GPIO_Init();                 /* 初始化有源蜂鸣器*/
    
    lcd_show_string(30, 20, 200, 16, 24, "STM32F407ZGT6", BLACK);
    lcd_show_string(30, 60, 200, 16, 24, "RTC TEST", BLACK);
    lcd_show_string(30, 90, 200, 16, 24, "Alone@Walker", RED);
    lcd_show_chinese(30, 270, "你好呀，小黑", 32, BLUE);
    /* 设置RTC周期性唤醒中断 */
    rtc_set_wakeup(RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0);
    //设置闹钟时间（可通过USMART调用）
    //rtc_set_alarm( 2,  22, 42 ,  0);
    while (1)
    {
      
        
        
        if ((t % 10) == 0)
        {
           
            //获取RTC日期信息 
            rtc_get_date(&year, &month, &date, &week);
            sprintf(tbuf, "Date:20%02d-%02d-%02d", year, month, date);
            lcd_show_string(30, 120, 210, 16, 24, tbuf, BLACK);
            
             // 获取RTC时间信息
            rtc_get_time(&hour, &minute, &second, &ampm);
            sprintf(tbuf, "Time:%02d:%02d:%02d", hour, minute, second);
            lcd_show_string(30, 150, 210, 16, 24, tbuf, RED);
            
            //获取周
            sprintf(tbuf, "Week:%s", week_str[week]);
            lcd_show_string(30, 190, 210, 16, 24, tbuf, BLACK);
            
            //获取闹钟时间
            rtc_get_alarm(&alarm_hour,&alarm_minute,&alarm_second,&alarm_week);
          
            sprintf(tbuf, "Alarm:%02d:%02d:%02d %s", alarm_hour, alarm_minute, alarm_second, week_str[alarm_week]);
            lcd_show_string(20, 230, 210, 16, 24, tbuf, RED);
            
            
        }
        
        if (++t == 20)
        {
            t = 0;
            LED0_TOGGLE();
        }
        
        /* 蜂鸣器状态机（每 10ms 检查一次，500ms = 50 次） */
        if (g_alarm_triggered)
        {
        buzzer_tick++;
        if (buzzer_tick >= 50)  // 500ms
        {
            buzzer_tick = 0;
            if (g_buzzer_count > 0)
            {
                g_buzzer_state = !g_buzzer_state;   // 翻转状态
                buzzer_switch(g_buzzer_state ? 0 : 1);  // 响/关
                g_buzzer_count--;
            }
            else
            {
                /* 响动结束 */
                buzzer_switch(1);  // 关闭蜂鸣器
                g_alarm_triggered = 0;
            }
        }
    }
        delay_ms(10);
        
    }
}
