#include "./SYSTEM/sys/sys.h"        // 系统核心头文件（类型定义、位操作、时钟配置等）
#include "./SYSTEM/delay/delay.h"    // 延时函数头文件（delay_ms、delay_us）
#include "./SYSTEM/usart/usart.h"    // 串口驱动头文件（用于调试信息输出）
#include "./BSP/LED/led-i.h"         // LED驱动头文件（LED0、LED1控制）
#include "./BSP/LCD/lcd.h"           // LCD驱动头文件（屏幕显示功能）
#include "./BSP/ADC/adc.h"           // ADC驱动头文件（含内部温度传感器采集函数）

/**
 * @brief   主函数
 * @param   无
 * @retval  无
 * @note    本实验通过 ADC1 的通道 16（内部温度传感器）采集芯片温度，
 *          并通过 LCD 显示温度值（单位：℃）。
 */
int main(void)
{
    int16_t temperature;   // 温度值（扩大100倍后的整数，如 25.00℃ → 2500）

    /* --- 系统初始化（按照依赖关系逐一初始化） --- */
    HAL_Init();                         // 初始化 HAL 库（必须最先调用）
    sys_stm32_clock_init(336, 8, 2, 7); // 配置系统时钟为 168MHz
    delay_init(168);                    // 初始化延时函数（基于 SysTick）
    usart_init(115200);                 // 初始化串口 1（波特率 115200）
    led_i_init();                       // 初始化 LED（GPIO 配置）
    lcd_init();                         // 初始化 LCD（FSMC + ST7789 驱动）
    adc_temperature_init();             // ★ 初始化 ADC 用于采集内部温度传感器

    /* --- LCD 显示固定标题信息 --- */
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);           // 第 1 行：开发板型号
    lcd_show_string(30, 70, 200, 16, 16, "Temperature TEST", RED); // 第 2 行：实验名称
    lcd_show_string(30, 90, 200, 16, 16, "ALONE@ALKER", RED);   // 第 3 行：作者/版权信息

    /* ★ 显示温度标签（固定文字） */
    // 第 4 行：显示 "TEMPERATE: 00.00C"，其中 "00.00" 会被实时更新
    lcd_show_string(30, 110, 200, 16, 16, "TEMPERATE: 00.00C", BLUE);

    /* --- ★ 主循环（程序的核心逻辑，每 100ms 执行一次） --- */
    while (1)
    {
        /* ① 获取内部温度传感器采样结果（单位：0.01℃） */
        // adc_get_temperature() 内部完成 ADC 采样、电压换算、温度计算
        // 返回值范围：-40.00℃ ~ 125.00℃（STM32F4 温度范围）
        temperature = adc_get_temperature();    /* 获取内部温度传感器结果 */

        /* ② 在 LCD 上显示温度值（格式：符号 + 整数部分 + 小数部分） */
        // 第 1 部分：显示符号（负数显示 "-"，正数显示空格）
        if (temperature < 0)
        {
            temperature = -temperature;   // 取绝对值（便于显示）
            // 在固定位置显示 "-" 号（坐标：30 + 10*8 = 110，正好在标签后面）
            lcd_show_string(30 + 10 * 8, 110, 16, 16, 16, "-", BLUE);
        }
        else
        {
            // 正数显示空格（覆盖可能残留的 "-" 号）
            lcd_show_string(30 + 10 * 8, 110, 16, 16, 16, " ", BLUE);
        }

        // 第 2 部分：显示整数部分（2 位数，如 25℃ 显示 "25"）
        // temperature / 100 得到整数部分（如 2500 / 100 = 25）
        lcd_show_xnum(30 + 11 * 8, 110, temperature / 100, 2, 16, 0, BLUE);

        // 第 3 部分：显示小数部分（2 位数，如 0.25℃ 显示 "25"）
        // temperature % 100 得到小数部分（如 2500 % 100 = 0 → 显示 "00"）
        // 0x80 表示高位补 0（确保显示 2 位，如 "05"）
        lcd_show_xnum(30 + 14 * 8, 110, temperature % 100, 2, 16, 0x80, BLUE);

        /* ③ LED0 翻转（指示程序正常运行） */
        // 每 100ms 翻转一次，人眼看到的是以 200ms 周期闪烁
        LED0_TOGGLE();

        /* ④ 延时 100ms（控制采样频率和 LED 闪烁速度） */
        // 每秒采样 10 次，实时显示温度变化
        delay_ms(100);
    }
}

