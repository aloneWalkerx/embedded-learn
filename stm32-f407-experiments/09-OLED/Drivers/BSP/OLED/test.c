#include "./BSP/OLED/oled.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/OLED/bmp.h"
#include "./BSP/OLED/test.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// 一个内部小函数：填充矩形（直接用循环画点）
static void OLED_FillRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    for (uint8_t y = y1; y <= y2; y++) {
        for (uint8_t x = x1; x <= x2; x++) {
            OLED_SetPixel(x, y, color);
        }
    }
}

// ============ 主界面 ============
void TEST_MainPage(void)
{
    OLED_Clear(0);
    OLED_ShowString(28, 0, "OLED-TEST", 16, 0);
    OLED_ShowString(20, 16, "\"1.3\" SH1106", 16, 1);
    OLED_ShowString(40, 32, "128*64", 16, 0);
    OLED_ShowString(4, 48, "www.aloneWalker.com", 8, 1);
   
    delay_ms(3000);   // 停留 3 秒
}

// ============ 刷屏测试（黑白切换） ============
void Test_Color(void)
{
    OLED_Clear(0);
    OLED_ShowString(10, 10, "BLACK", 16, 1);
    delay_ms(1000);
    OLED_Clear(1);    // 全白
    delay_ms(1500);
    OLED_Clear(0);    // 恢复黑
}

// ============ 矩形绘制和填充 ============
void Test_Rectangular(void)
{
    OLED_FillRect(0, 0, 63, 63, 0);     // 左半黑背景
    OLED_FillRect(64, 0, 127, 63, 1);   // 右半白背景
    OLED_DrawRect(5, 5, 58, 58, 1);     // 左半画白框
    OLED_DrawRect(69, 5, 122, 58, 0);   // 右半画黑框
    OLED_Display();
    delay_ms(1000);
    OLED_FillRect(5, 5, 58, 58, 1);     // 填充左矩形白
    OLED_FillRect(69, 5, 122, 58, 0);   // 填充右矩形黑
    OLED_Display();
    delay_ms(1500);
}

// ============ 圆形绘制 ============
void Test_Circle(void)
{
    OLED_FillRect(0, 0, 63, 63, 0);
    OLED_FillRect(64, 0, 127, 63, 1);
    OLED_DrawCircle(31, 31, 27, 1);     // 左圆
    OLED_DrawCircle(95, 31, 27, 0);     // 右圆
    OLED_Display();
    delay_ms(1000);
    // 从外到内画同心圆（实现填充效果）
    for (int r = 27; r >= 0; r--) {
        OLED_DrawCircle(31, 31, r, 1);
    }
    for (int r = 27; r >= 0; r--) {
        OLED_DrawCircle(95, 31, r, 0);
    }
    OLED_Display();
    delay_ms(1500);
}

// ============ 三角形绘制和填充 ============
void Test_Triangle(void)
{
    OLED_FillRect(0, 0, 63, 63, 0);
    OLED_FillRect(64, 0, 127, 63, 1);
    OLED_DrawTriangle(5, 58, 31, 5, 58, 58, 1);   // 左三角形
    OLED_DrawTriangle(69, 58, 95, 5, 122, 58, 0); // 右三角形
    OLED_Display();
    delay_ms(1000);
    // 用水平线填充三角形（扫描线算法）
    for (uint8_t y = 5; y <= 58; y++) {
        uint8_t x_left = 5 + (31 - 5) * (y - 5) / (58 - 5);
        uint8_t x_right = 58 - (58 - 31) * (58 - y) / (58 - 5);
        OLED_DrawLine(x_left, y, x_right, y, 1);
    }
    for (uint8_t y = 5; y <= 58; y++) {
        uint8_t x_left = 69 + (95 - 69) * (y - 5) / (58 - 5);
        uint8_t x_right = 122 - (122 - 95) * (58 - y) / (58 - 5);
        OLED_DrawLine(x_left, y, x_right, y, 0);
    }
    OLED_Display();
    delay_ms(1500);
}

// ============ 英文字符显示 ============
void TEST_English(void)
{
    OLED_Clear(0);
    OLED_ShowString(0, 5, "6x8:abcdefghijklmnopqrstuvwxyz", 8, 1);
    OLED_ShowString(0, 25, "8x16:abcdefghijklmnopqrstuvwxyz", 16, 1);
    delay_ms(1000);
    OLED_Clear(0);
    OLED_ShowString(0, 5, "6x8:ABCDEFGHIJKLMNOPQRSTUVWXYZ", 8, 1);
    OLED_ShowString(0, 25, "8x16:ABCDEFGHIJKLMNOPQRSTUVWXYZ", 16, 1);
    delay_ms(1500);
}

// ============ 数字和特殊字符 ============
void TEST_Number_Character(void)
{
    OLED_Clear(0);
    OLED_ShowString(0, 0, "6x8:!\"#$%&'()*+,-./:;<=>?@[]\\^_`~{}|", 8, 1);
    OLED_ShowNum(30, 16, 1234567890, 10, 8, 1);
    delay_ms(1000);
    OLED_Clear(0);
    OLED_ShowString(0, 0, "8x16:!\"#$%&'()*+,-./:;<=>?@[]\\^_`~{}|", 16, 1);
    OLED_ShowNum(40, 32, 1234567890, 10, 16, 1);
    delay_ms(1500);
}

// ============ 中文显示 ============
void TEST_Chinese(void)
{
    OLED_Clear(0);
    OLED_ShowString(45, 0, "16x16", 8, 1);
    OLED_ShowChinese(16, 20, "你好呀，小黑", 16, 1);   // 注意：字库中必须按这个顺序
    OLED_Display();
    delay_ms(1000);
    OLED_Clear(0);
    OLED_ShowString(45, 0, "24x24", 8, 1);
    OLED_ShowChinese(16, 20, "你好呀，", 24, 1);
    OLED_Display();
    delay_ms(1000);
    OLED_Clear(0);
    OLED_ShowString(45, 0, "32x32", 8, 1);
    OLED_ShowChinese(0, 20, "你好呀，", 32, 1);
    OLED_Display();
    delay_ms(1000);
}

// ============ BMP 图片显示 ============
void TEST_BMP(void)
{
    OLED_Clear(0);
    OLED_ShowBMP(0, 0, 128, 64, BMP2, 1);
    OLED_Display();
    delay_ms(1000);
    OLED_ShowBMP(0, 0, 128, 64, BMP3, 1);
    OLED_Display();
    delay_ms(1000);
    OLED_ShowBMP(0, 0, 128, 64, BMP4, 1);
    OLED_Display();
    delay_ms(1000);
}

// ============ 菜单1（圆形选择器） ============
void TEST_Menu1(void)
{
    OLED_Clear(0);
    OLED_FillRect(0, 0, 127, 15, 1);   // 顶部白条
    OLED_ShowString(32, 0, "System", 16, 0);
    // 选项 A
    OLED_DrawCircle(10, 24, 6, 1);
    OLED_DrawCircle(10, 24, 3, 1);
    OLED_ShowString(20, 16, "A.Volume", 16, 1);
    // 选项 B
    OLED_DrawCircle(10, 40, 6, 1);
    OLED_DrawCircle(10, 40, 3, 1);
    OLED_ShowString(20, 32, "B.Color", 16, 1);
    // 选项 C
    OLED_DrawCircle(10, 56, 6, 1);
    OLED_DrawCircle(10, 56, 3, 1);
    OLED_ShowString(20, 48, "C.Network", 16, 1);
    // 右侧滚动条
    OLED_DrawRect(0, 0, 127, 63, 1);
    OLED_DrawLine(117, 15, 117, 63, 1);
    OLED_FillTriangel(118, 20, 122, 16, 126, 20, 1);   // 上三角
    OLED_FillTriangel(118, 54, 122, 58, 126, 54, 1);   // 下三角
    // 默认选中 A
    OLED_FillCircle(10, 24, 1, 3);   // 填充小圆（表示选中）
    OLED_Display();
    delay_ms(1500);
    // 移到 B
    OLED_FillCircle(10, 24, 0, 3);
    OLED_DrawCircle(10, 24, 1, 3);   // 恢复空心
    OLED_FillCircle(10, 40, 1, 3);
    OLED_Display();
    delay_ms(1500);
    // 移到 C
    OLED_FillCircle(10, 40, 0, 3);
    OLED_DrawCircle(10, 40, 1, 3);
    OLED_FillCircle(10, 56, 1, 3);
    OLED_Display();
    delay_ms(1500);
}

// ============ 菜单2（天气界面，动态刷新数据） ============
void TEST_Menu2(void)
{
    OLED_Clear(0);
    OLED_ShowString(0, 1, "2026-08-28", 8, 1);
    OLED_ShowString(78, 1, "Saturday", 8, 1);
    OLED_DrawLine(0, 10, 127, 10, 1);
    OLED_ShowBMP(6, 16, 51, 32, BMP5, 1);   // 显示小云朵图标
    OLED_ShowString(14, 54, "Cloudy", 8, 1);
    OLED_DrawLine(63, 10, 63, 63, 1);      // 中间分隔线
    OLED_ShowString(70, 13, "TEMP", 8, 1);
    OLED_DrawCircle(108, 25, 2, 1);         // 温度符号的小圈
    OLED_ShowString(114, 20, "C", 16, 1);
    OLED_ShowString(74, 20, "32.5", 16, 1);
    OLED_ShowString(70, 39, "PM2.5", 8, 1);
    OLED_ShowString(72, 46, "90ug/m3", 16, 1);
    OLED_DrawLine(63, 35, 127, 35, 1);
    OLED_Display();
    delay_ms(500);

    // 模拟温度、PM2.5 变化（随机数）
    srand(123456);
    for (int i = 0; i < 15; i++) {
        uint8_t temp_int = rand() % 4;
        uint8_t temp_dec1 = rand() % 10;
        uint8_t temp_dec2 = rand() % 10;
        OLED_ShowNum(74, 20, temp_int, 1, 16, 1);
        OLED_ShowNum(82, 20, temp_dec1, 1, 16, 1);
        OLED_ShowNum(90, 20, temp_dec2, 1, 16, 1);
        uint8_t pm1 = rand() % 10;
        uint8_t pm2 = rand() % 10;
        OLED_ShowNum(72, 46, pm1, 1, 16, 1);
        OLED_ShowNum(80, 46, pm2, 1, 16, 1);
        OLED_Display();
        delay_ms(500);
    }
}

