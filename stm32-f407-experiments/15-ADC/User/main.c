#include "./SYSTEM/sys/sys.h"        // 系统核心头文件（类型定义、位操作、时钟配置等）
#include "./SYSTEM/delay/delay.h"    // 延时函数头文件（delay_ms、delay_us）
#include "./SYSTEM/usart/usart.h"    // 串口驱动头文件（用于调试信息输出）
#include "./BSP/LED/led-i.h"           // LED驱动头文件（LED0、LED1控制）
#include "./BSP/LCD/lcd.h"           // LCD驱动头文件（屏幕显示功能）
#include "./BSP/ADC/adc.h"           // ADC驱动头文件（ADC初始化、读取函数）

/**
 * @brief   主函数
 * @param   无
 * @retval  无
 * @note    程序入口，执行顺序如下：
 *          1. 初始化所有外设（HAL、时钟、延时、串口、LED、LCD、ADC）
 *          2. 在 LCD 上显示固定标题信息
 *          3. 进入主循环，每 100ms 读取一次 ADC 值并显示在 LCD 上
 *          4. LED0 每 100ms 翻转一次，指示程序正在运行
 */
int main(void)
{
    /* --- 局部变量定义 --- */
    uint16_t adc_result;   // ADC 原始转换结果（0~4095）
    uint16_t voltage;      // 电压值（单位：mV，扩大 1000 倍后的整数）
    
    /* --- 系统初始化（按照依赖关系逐一初始化） --- */
    HAL_Init();                         // 初始化 HAL 库（必须最先调用）
    sys_stm32_clock_init(336, 8, 2, 7); // 配置系统时钟为 168MHz
    delay_init(168);                    // 初始化延时函数（基于 SysTick）
    usart_init(115200);                 // 初始化串口 1（波特率 115200）
    led_i_init();                         // 初始化 LED（GPIO 配置）
    lcd_init();                         // 初始化 LCD（FSMC + ST7789 驱动）
    adc_init();                         // ★ 初始化 ADC（配置 ADC2、通道 1、PA1）
    
    /* --- LCD 显示固定标题信息 --- */
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);           // 第 1 行：开发板型号
    lcd_show_string(30, 70, 200, 16, 16, "ADC TEST", RED);        // 第 2 行：实验名称
    lcd_show_string(30, 90, 200, 16, 16, "Alone@Walker", RED);   // 第 3 行：作者/版权信息
    
    /* ★ 显示 ADC 数值的标签（固定文字） */
    // 第 4 行：显示 "ADC2_CH1_VAL:0"，其中 "0" 会被实时更新
    lcd_show_string(30, 110, 200, 16, 16, "ADC2_CH1_VAL:0", BLUE);
    
    // 第 5 行：显示 "ADC2_CH1_VOL:0.000V"，其中 "0.000" 会被实时更新
    lcd_show_string(30, 130, 200, 16, 16, "ADC2_CH1_VOL:0.000V", BLUE);
    
    /* --- ★ 主循环（程序的核心逻辑，每 100ms 执行一次） --- */
    while (1)
    {
        /* ① 读取 ADC 值（均值滤波） */
        // ★ adc_get_result_average 会采样 10 次取平均值
        // 参数 ADC_ADCX_CHY 是 adc.h 中定义的通道宏（ADC_CHANNEL_1）
        // 返回值范围：0 ~ 4095（12 位 ADC）
        adc_result = adc_get_result_average(ADC_ADCX_CHY, 10);
        
        /* ② 在 LCD 上显示 ADC 原始值 */
        // lcd_show_xnum 是扩展数字显示函数
        // 参数说明：
        //   - 134：X 坐标（显示在标签后面）
        //   - 110：Y 坐标
        //   - adc_result：要显示的数字
        //   - 5：总显示位数（不足 5 位时补空格或补 0）
        //   - 16：字体大小
        //   - 0：模式（0 表示高位补空格，0x80 表示高位补 0）
        //   - BLUE：颜色
        lcd_show_xnum(134, 110, adc_result, 5, 16, 0, BLUE);
        
        /* ③ 计算实际电压值 */
        // ★ 电压计算公式：V = (ADC值 / 4095) × 3.3V
        // 为了在 LCD 上显示小数，先乘以 3300 得到毫伏值（扩大 1000 倍）
        // 例如：ADC = 2048 → (2048 × 3300) / 4095 ≈ 1650mV = 1.650V
        voltage = (adc_result * 3300) / 4095;
        
        /* ④ 在 LCD 上显示电压值 */
        // 第 1 部分：显示整数部分（单位：伏特）
        // voltage / 1000 得到整数部分（如 1.650V 中的 1）
        lcd_show_xnum(134, 130, voltage / 1000, 1, 16, 0, BLUE);
        
        // 显示小数点（用字符串 " " 覆盖占位）
        // 注意：这里没有直接显示 "."，而是用空格覆盖了原来的位置
        // 因为 lcd_show_xnum 不能显示小数点，所以用固定字符串方式显示
        // 实际效果：先显示整数部分，然后"."是固定的，再显示小数部分
        
        // 第 2 部分：显示小数部分（3 位）
        // voltage % 1000 取余数得到小数部分（如 1.650V 中的 650）
        // 0x80 表示高位补 0（确保显示 3 位，如 "650"）
        lcd_show_xnum(150, 130, voltage % 1000, 3, 16, 0x80, BLUE);
        
        /* ⑤ LED0 翻转（指示程序正常运行） */
        // 每 100ms 翻转一次 LED0，人眼看到的是以 200ms 周期闪烁（亮 100ms，灭 100ms）
        LED0_TOGGLE();
        
        /* ⑥ 延时 100ms（控制采样频率和 LED 闪烁速度） */
        // 100ms 采样一次，即每秒采样 10 次
        // 这个速度适合观察电压变化（如电位器调节、光照变化等）
        delay_ms(100);
    }
}
