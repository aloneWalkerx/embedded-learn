#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/KEY/key-i.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/24CXX/24cxx.h"

static const uint8_t g_text_buf[] = {"STM32 IIC TEST"};
#define TEXT_SIZE sizeof(g_text_buf)
    

int main(void)
{ 
    uint8_t t = 0;
    uint8_t key;
    uint8_t data[TEXT_SIZE];
    
    
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                         /* 初始化LED */
    key_i_init();                         /* 初始化按键 */
    lcd_init();                         /* 初始化LCD */
    at24cxx_init();                     /* 初始化AT24CXX */
    
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "IIC TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    
    lcd_show_string(30, 110, 200, 16, 16, "WK_UP:Write KEY0:Read", RED);
    
     /* 检测AT24CXX是否正常 */
    while (at24cxx_check() != 0)
    {
      
        lcd_show_string(30, 130, 200, 16, 16, "24C02 Check Failed!", RED);
        delay_ms(500);
        lcd_show_string(30, 130, 200, 16, 16, "Please Check!      ", RED);
        delay_ms(500);
        LED0_TOGGLE();  
      
        
    }
    
        
    while (1)
    {
        key = key_i_scan(0);
        if (key == WKUP_PRESS)
        {
            /* 往AT24CXX写入数据 */
            lcd_fill(0, 150, 239, 319, WHITE); 
            lcd_show_string(30, 150, 200, 16, 16, "Start Write 24C02....", BLUE);
            at24cxx_write(0, (uint8_t *)g_text_buf, TEXT_SIZE);
            lcd_show_string(30, 150, 200, 16, 16, "24C02 Write Finished!", BLUE);
        }
        else if (key == KEY0_PRESS)
        {
            /* 从AT24CXX读取数据 */
            lcd_show_string(30, 150, 200, 16, 16, "Start Read 24C02.... ", BLUE);
            at24cxx_read(0, data, TEXT_SIZE);
            lcd_show_string(30, 150, 200, 16, 16, "The Data Readed Is:  ", BLUE);
            lcd_show_string(30, 170, 200, 16, 16, (char *)data, BLUE);
        }
        
        if (++t == 20)
        {
            t = 0;
            LED0_TOGGLE();
        }
        
        delay_ms(10);
    }
}
