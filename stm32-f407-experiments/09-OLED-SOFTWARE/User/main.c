#include "./SYSTEM/sys/sys.h"         // 系统基础类型定义（比如 uint8_t、uint32_t 等）
#include "./SYSTEM/delay/delay.h"     // 延时函数（用于等待、动画间隔）
#include "./SYSTEM/usart/usart.h"     // 串口初始化（调试用，可以打印信息到电脑）
#include "./BSP/LED/led-i.h"          // 板载 LED 灯驱动（可能用来指示状态）
#include "./BSP/OLED/oled.h"          // OLED 显示屏驱动（画图、显示文字）
#include "./BSP/OLED/test.h"          // 测试函数集合（各种演示效果）

// ================================================================
// 主函数 —— 程序一上电就从这里开始执行
// ================================================================
int main(void)
{
    // -------- 第一步：STM32 底层硬件初始化（必须的） --------
    HAL_Init();                         // 初始化 HAL 库（STM32 的硬件抽象层）
    sys_stm32_clock_init(336, 8, 2, 7); // 配置系统时钟：主频 168MHz（参数是固定的，不用纠结）
    delay_init(168);                    // 初始化延时函数，参数是 CPU 频率（168MHz）
    usart_init(115200);                 // 初始化串口，波特率 115200（用于和电脑通信调试）
    led_i_init();                       // 初始化板载 LED（比如点亮一下表示程序运行了）

    // -------- 第二步：OLED 显示屏初始化 --------
    OLED_Init();                        // 给 OLED 发送一堆初始化命令，让它进入工作状态
    OLED_Clear(0);                      // 清屏为黑色（0 表示黑色，1 表示白色）

    // -------- 第三步：大循环 —— 不停地轮流显示各种测试效果 --------
    while (1)                           // 死循环，程序一直在这里跑
    {
        // ① 显示主界面（LOGO 信息）
        TEST_MainPage();                // 显示 "OLED TEST" 等文字，停留 3 秒
        OLED_Clear(0);                  // 清屏，准备下一个测试
        delay_ms(500);                  // 稍作停顿，避免切换太快

        // ② 刷屏测试（黑白交替）
        Test_Color();                   // 显示 "BLACK" 然后全白再全黑
        OLED_Clear(0);
        delay_ms(500);

        // ③ 矩形绘制和填充测试
        Test_Rectangular();             // 左右半屏不同底色，画矩形框并填充
        OLED_Clear(0);
        delay_ms(500);

        // ④ 圆形绘制测试（外圈到内圈填充）
        Test_Circle();                  // 画两个大圆，然后从外到内画同心圆填充
        OLED_Clear(0);
        delay_ms(500);

        // ⑤ 三角形绘制和填充测试
        Test_Triangle();                // 画两个三角形，再用水平线填充
        OLED_Clear(0);
        delay_ms(500);

        // ⑥ 英文字符显示（大小写字母）
        TEST_English();                 // 显示 6x8 和 8x16 两套英文字母
        OLED_Clear(0);
        delay_ms(500);

        // ⑦ 数字和特殊符号显示
        TEST_Number_Character();        // 显示各种符号和 1234567890
        OLED_Clear(0);
        delay_ms(500);

        // ⑧ 中文字符显示（三种字号）
        TEST_Chinese();                 // 显示“你好呀，小黑” 16/24/32 点阵
        OLED_Clear(0);
        delay_ms(500);

        // ⑨ BMP 图片显示（内置的几幅单色图）
        TEST_BMP();                     // 轮流显示 BMP2、BMP3、BMP4
        OLED_Clear(0);
        delay_ms(500);

        // ⑩ 菜单界面 1（带圆形选择器）
        TEST_Menu1();                   // 显示 System 菜单，模拟选项切换
        OLED_Clear(0);
        delay_ms(500);

        // ? 菜单界面 2（天气风格，动态刷新温度/PM2.5）
        TEST_Menu2();                   // 显示日期、云朵图标、温度、PM2.5 并随机变化
        OLED_Clear(0);
        delay_ms(500);
    }
}

