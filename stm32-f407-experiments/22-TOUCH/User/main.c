#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/KEY/key-i.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/TOUCH/touch.h"

/**
 * @brief   清空屏幕并在右上角显示"RST"
 * @param   无
 * @retval  无
 */
static void load_draw_dialog(void)
{
    lcd_clear(WHITE);                                                /* 清屏 */
    lcd_show_string(lcddev.width - 24, 0, 200, 16, 16, "RST", BLUE); /* 显示清屏区域 */
}



/**
 * @brief   电阻触摸屏测试
 * @param   无
 * @retval  无
 */
void rtp_test(void)
{
    uint8_t key;
    uint8_t i = 0;
    
    while (1)
    {
        key = key_i_scan(0);
        tp_dev.scan(0);
        
        if (tp_dev.sta & TP_PRES_DOWN)                                          /* 触摸屏被按下 */
        {
            if ((tp_dev.x[0] < lcddev.width) && (tp_dev.y[0] < lcddev.height))
            {
                if ((tp_dev.x[0] > (lcddev.width - 24)) && (tp_dev.y[0] < 16))
                {
                    load_draw_dialog();                                         /* 清除 */
                }
                else
                {
                    tp_draw_big_point(tp_dev.x[0], tp_dev.y[0], RED);           /* 画点 */
                }
            }
        }
        else
        {
            delay_ms(10);                                                       /* 没有按键按下的时候 */
        }
        
        if (key == KEY0_PRESS)                                                   /* KEY0按下，则执行校准程序 */
        {
            tp_adjust();                                                        /* 屏幕校准 */
            tp_save_adjust_data();
            load_draw_dialog();
        }
        
        i++;
        if ((i % 20) == 0)
        {
            LED0_TOGGLE();
        }
    }
}


int main(void)
{
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                       /* 初始化LED */
    key_i_init();                       /* 初始化按键 */
    lcd_init();                         /* 初始化LCD */
    tp_dev.init();                      /* 初始化触摸屏 */
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "TOUCH TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ALONE@ALKER", RED);
    
    /* 电阻屏显示触摸校准提示 */
    if ((tp_dev.touchtype & 0x80) == 0)
    {
        lcd_show_string(30, 110, 200, 16, 16, "Press KEY0 to Adjust", RED);
    }
    delay_ms(1500);
    load_draw_dialog();
    
   /* 电阻屏测试 */
        rtp_test();
    
}
