#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/LCD/lcd.h"
int main(void)
{ 
    uint8_t x = 0;
    uint8_t lcd_id[13];
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                         /* 初始化LED */
    lcd_init();                         /* 初始化LCD */
    sprintf((char *)lcd_id, "LCD ID: %04X", lcddev.id);
    
    
    
    while (1)
    {
      
     /* ① 清屏为白色，显示标题 */
    lcd_clear(WHITE);
    lcd_show_string(10, 10, 240, 32, 32, "TFTLCD TEST", RED);
    lcd_show_string(10, 50, 240, 24, 24, (char *)lcd_id, BLUE);
    delay_ms(2000);

    /* ② 测试画点、画线、矩形、圆、填充圆 */
    lcd_clear(BLACK);
    // 画点：在 (100,100) 画一个红点
    lcd_draw_point(100, 100, RED);
    // 画线：从 (20,20) 到 (220,180)
    lcd_draw_line(20, 20, 220, 180, GREEN);
    // 画水平线：从 (30, 150) 长度 200
    lcd_draw_hline(30, 150, 200, CYAN);
    // 画矩形：左上(50,50) 右下(150,150)
    lcd_draw_rectangle(50, 50, 150, 150, YELLOW);
    // 画圆：圆心 (100,100) 半径 40
    lcd_draw_circle(100, 100, 40, MAGENTA);
    // 填充圆：圆心 (180,180) 半径 30
    lcd_fill_circle(180, 180, 30, BLUE);
    // 显示一些文字说明
    lcd_show_string(10, 210, 240, 16, 16, "Draw Test OK", RED);
    delay_ms(3000);

    /* ③ 测试区域填充（lcd_fill 和 lcd_color_fill） */
    lcd_clear(WHITE);
    // 填充一个矩形区域为红色
    lcd_fill(20, 20, 100, 100, RED);
    // 填充一个矩形区域为绿色
    lcd_fill(120, 20, 200, 100, GREEN);
    // 使用颜色数组填充一块区域（模拟渐变色）
    {
        uint16_t color_buf[10*10];
        uint16_t i, j;
        for (i = 0; i < 10; i++) {
            for (j = 0; j < 10; j++) {
                // 生成简单的渐变色（蓝-紫）
                color_buf[i*10 + j] = BLUE + (i * 0x001F) + (j * 0x001F);
            }
        }
        lcd_color_fill(30, 130, 129, 229, color_buf);
    }
    lcd_show_string(10, 240, 240, 16, 16, "Fill Test OK", BLUE);
    delay_ms(3000);

    /* ④ 测试字符、数字、字符串显示（所有字体大小） */
    lcd_clear(BLACK);
    // 12x12 字符
    lcd_show_char(10, 10, 'A', 12, 0, RED);
    lcd_show_char(30, 10, 'B', 12, 1, GREEN);  // 叠加模式
    // 16x16 字符
    lcd_show_char(10, 30, 'C', 16, 0, YELLOW);
    // 24x24 字符
    lcd_show_char(10, 60, 'D', 24, 0, CYAN);
    // 32x32 字符
    lcd_show_char(10, 100, 'E', 32, 0, MAGENTA);

    // 显示数字（普通、扩展）
    lcd_show_num(10, 150, 1234567890, 10, 16, BLUE);
    lcd_show_xnum(10, 180, 9876, 6, 16, 0x80, GREEN);  // 高位补0

    // 显示字符串（自动换行）
    lcd_show_string(10, 210, 240, 320, 16, "String Test: Hello TFTLCD!", RED);
    delay_ms(3000);

    /* ⑤ 测试窗口设置（使用 lcd_set_window 配合 lcd_fill） */
    lcd_clear(WHITE);
    // 设置窗口为 (50,50) 到 (150,150)，然后填充该窗口为蓝色
    lcd_set_window(50, 50, 100, 100);
    lcd_fill(50, 50, 150, 150, BLUE);   // 注意：实际填充必须指定坐标，但窗口限制了写入区域
    // 重置窗口为全屏（不显式重置，lcd_fill 会重新设置坐标，但窗口保持生效直到重新设置）
    // 为了演示，再设置全屏窗口
    lcd_set_window(0, 0, lcddev.width, lcddev.height);
    lcd_show_string(10, 220, 240, 16, 16, "Window Test OK", RED);
    delay_ms(3000);

    /* ⑥ 测试扫描方向与显示方向 */
    lcd_clear(BLACK);
    lcd_show_string(10, 10, 240, 16, 16, "Scan Dir: L2R_U2D", WHITE);
    lcd_scan_dir(L2R_U2D);
    delay_ms(1500);

    lcd_clear(BLACK);
    lcd_show_string(10, 10, 240, 16, 16, "Scan Dir: R2L_D2U", WHITE);
    lcd_scan_dir(R2L_D2U);
    delay_ms(1500);

    lcd_clear(BLACK);
    lcd_show_string(10, 10, 240, 16, 16, "Display Dir: Landscape", WHITE);
    lcd_display_dir(1);     // 横屏
    delay_ms(1500);

    lcd_clear(BLACK);
    lcd_show_string(10, 10, 240, 16, 16, "Display Dir: Portrait", WHITE);
    lcd_display_dir(0);     // 竖屏（恢复）
    delay_ms(1500);

    /* ⑦ 测试背光控制（如果用的是普通GPIO） */
    // 关闭背光
    LCD_BL(0);
    delay_ms(1000);
    // 打开背光
    LCD_BL(1);
    delay_ms(500);

    /* ⑧ 读取某点颜色并显示 */
    uint16_t point_color = lcd_read_point(50, 50);
    lcd_clear(BLACK);
    lcd_show_num(10, 10, point_color, 4, 16, WHITE);
    lcd_show_string(10, 40, 240, 16, 16, "Point Color Read OK", GREEN);
    delay_ms(2000);

    /* ⑨ 测试开关显示 */
    lcd_display_off();
    delay_ms(1000);
    lcd_display_on();
    delay_ms(500);

    /* ⑩ 清屏为白色，显示最终信息 */
    lcd_clear(WHITE);
    lcd_show_string(10, 10, 240, 32, 32, "All Tests Done!", RED);
    lcd_show_string(10, 60, 240, 24, 24, (char *)lcd_id, BLUE);
    delay_ms(3000);  
        
        
        switch (x)
        {
            case 0:
            {
                lcd_clear(WHITE);
                break;
            }
            case 1:
            {
                lcd_clear(BLACK);
                break;
            }
            case 2:
            {
                lcd_clear(BLUE);
                break;
            }
            case 3:
            {
                lcd_clear(RED);
                break;
            }
            case 4:
            {
                lcd_clear(MAGENTA);
                break;
            }
            case 5:
            {
                lcd_clear(GREEN);
                break;
            }
            case 6:
            {
                lcd_clear(CYAN);
                break;
            }
            case 7:
            {
                lcd_clear(YELLOW);
                break;
            }
            case 8:
            {
                lcd_clear(BRRED);
                break;
            }
            case 9:
            {
                lcd_clear(GRAY);
                break;
            }
            case 10:
            {
                lcd_clear(LGRAY);
                break;
            }
            case 11:
            {
                lcd_clear(BROWN);
                break;
            }
            
             case 12:
            {
                lcd_clear(LIGHTGREEN);
                break;
            }
            
        }
        
        lcd_show_string(10, 40, 240, 32, 32, "STM32", RED);
        lcd_show_string(10, 80, 240, 24, 24, "TFTLCD TEST", RED);
        lcd_show_string(10, 110, 240, 16, 16, "ATOM@ALIENTEK", RED);
        lcd_show_string(10, 130, 240, 16, 16, (char *)lcd_id, RED);
        
        if (++x == 13)
        {
            x = 0;
        }
        
        LED0_TOGGLE();
        delay_ms(1000);
    }
}
