#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/ADC/adc.h"

/* ====================================================================
   宏定义与全局变量
   ==================================================================== */

// ★ DMA 缓冲区大小（一次连续采样的数据个数）
// 每次 DMA 传输完成后，会采集 50 个 ADC 值
// 50 个值取平均，可以有效滤除随机噪声
#define ADC_DMA_BUF_SIZE    50

// ★ DMA 数据缓冲区（用于存储 ADC 采样结果）
// 缓冲区大小 = 50 个 uint16_t 数据
// DMA 会自动将 ADC_DR 的数据填入此数组
uint16_t g_adc_dma_buf[ADC_DMA_BUF_SIZE];

// ★ 外部变量声明：ADC DMA 转换完成标志
// 在 adc.c 中定义，由中断回调函数置位
// 1 = 转换完成，0 = 转换未完成
extern uint8_t g_adc_dma_sta;

/* ====================================================================
   主函数
   ==================================================================== */
int main(void)
{
    /* --- 局部变量定义 --- */
    uint16_t adc_result;   // ADC 原始转换结果（0~4095）
    uint16_t voltage;      // 电压值（单位：mV，扩大 1000 倍后的整数）
    uint16_t index;        // 循环索引变量
    uint32_t result_sum;   // 累加器（用于均值滤波）
    
    /* --- 系统初始化（按照依赖关系逐一初始化） --- */
    HAL_Init();                         // 初始化 HAL 库（必须最先调用）
    sys_stm32_clock_init(336, 8, 2, 7); // 配置系统时钟为 168MHz
    delay_init(168);                    // 初始化延时函数（基于 SysTick）
    usart_init(115200);                 // 初始化串口 1（波特率 115200）
    led_i_init();                       // 初始化 LED（GPIO 配置）
    lcd_init();                         // 初始化 LCD（FSMC + ST7789 驱动）
    
    /* ★ 初始化 ADC + DMA（关键步骤） */
    // 传入 DMA 缓冲区的首地址
    // 这样 DMA 就知道要把 ADC 数据搬运到哪里去
    adc_dma_init((uint32_t)g_adc_dma_buf);
    
    /* --- LCD 显示固定标题信息 --- */
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);           // 第 1 行：开发板型号
    lcd_show_string(30, 70, 200, 16, 16, "ADC DMA TEST", RED);    // 第 2 行：实验名称
    lcd_show_string(30, 90, 200, 16, 16, "Alone@Walker", RED);   // 第 3 行：作者/版权信息
    
    /* ★ 显示 ADC 数值的标签（固定文字） */
    // 第 4 行：显示 "ADC3_CH1_VAL:0"，其中 "0" 会被实时更新
    lcd_show_string(30, 110, 200, 16, 16, "ADC3_CH1_VAL:0", BLUE);
    
    // 第 5 行：显示 "ADC3_CH1_VOL:0.000V"，其中 "0.000" 会被实时更新
    lcd_show_string(30, 130, 200, 16, 16, "ADC3_CH1_VOL:0.000V", BLUE);
    
    /* ★ 启动 ADC + DMA 连续采样（核心操作！） */
    // 执行这条语句后：
    // ① ADC3 开始连续转换（ContinuousConvMode = ENABLE）
    // ② 每次转换完成 → 触发 DMA 请求
    // ③ DMA2_Stream1 自动从 ADC_DR 读取数据 → 存入 g_adc_dma_buf
    // ④ 内存地址自动递增，依次填充数组
    // ⑤ 采集完 50 个数据后，触发 DMA 传输完成中断
    // ⑥ 中断回调中置位 g_adc_dma_sta = 1
    // 
    // ★ 在 DMA 采集期间，CPU 完全自由！
    //    可以处理其他任务（如 LED 闪烁、按键扫描等）
    adc_dma_enable(ADC_DMA_BUF_SIZE);
    
    /* --- ★ 主循环（程序的核心逻辑） --- */
    while (1)
    {
        /* ① 检查 DMA 转换完成标志 */
        // 当 g_adc_dma_sta == 1 时，说明 DMA 已经采集了 50 个数据
        // 主循环可以开始处理这批数据了
        if (g_adc_dma_sta == 1)
        {
            // ★ 清除标志位（准备下一次采集）
            // 必须在处理数据前清零，否则主循环会反复处理同一批数据
            g_adc_dma_sta = 0;
            
            /* ② 均值滤波：对 50 个 ADC 值求和 */
            // result_sum = 累加结果，index = 循环计数器
            // for 循环的标准写法：初始值; 条件; 更新
            for (result_sum = 0, index = 0; index < ADC_DMA_BUF_SIZE; index++)
            {
                // 累加 DMA 缓冲区中的每个数据
                // g_adc_dma_buf[index] 是第 index 次采样的 ADC 值
                result_sum += g_adc_dma_buf[index];
            }
            
            /* ③ 计算平均值 */
            // 50 个数据取平均，消除随机噪声
            // 这样得到的 adc_result 比单次采样更稳定
            adc_result = result_sum / ADC_DMA_BUF_SIZE;
            
            /* ④ 在 LCD 上显示 ADC 原始值 */
            // lcd_show_xnum 是扩展数字显示函数
            // 参数：X坐标, Y坐标, 数值, 位数, 字号, 模式, 颜色
            // 模式 0 = 高位补空格，0x80 = 高位补 0
            lcd_show_xnum(134, 110, adc_result, 5, 16, 0, BLUE);
            
            /* ⑤ 计算实际电压值 */
            // ★ 电压计算公式：V = (ADC值 / 4095) × 3300mV
            // 为了在 LCD 上显示小数，先乘以 3300 得到毫伏值（扩大 1000 倍）
            // 例如：ADC = 2048 → (2048 × 3300) / 4095 ≈ 1650mV = 1.650V
            voltage = (adc_result * 3300) / 4095;
            
            /* ⑥ 在 LCD 上显示电压值 */
            // 第 1 部分：显示整数部分（单位：伏特）
            // voltage / 1000 得到整数部分（如 1.650V 中的 1）
            lcd_show_xnum(134, 130, voltage / 1000, 1, 16, 0, BLUE);
            
            // 第 2 部分：显示小数部分（3 位）
            // voltage % 1000 取余数得到小数部分（如 1.650V 中的 650）
            // 0x80 表示高位补 0（确保显示 3 位，如 "650"）
            lcd_show_xnum(150, 130, voltage % 1000, 3, 16, 0x80, BLUE);
            
            /* ⑦ 重新启动下一次 DMA 采集 */
            // ★ 为什么需要重新启动？
            // 因为 DMA 配置为 DMA_NORMAL（普通模式）
            // 采集完 50 个数据后，DMA 自动停止
            // 所以需要手动调用 adc_dma_enable 启动下一次采集
            // 如果配置为 DMA_CIRCULAR（循环模式），就不需要这行了
            adc_dma_enable(ADC_DMA_BUF_SIZE);
        }
        
        /* ⑧ LED0 翻转（指示程序正常运行） */
        // 每 100ms 翻转一次 LED0
        // 即使在 DMA 采集期间，LED0 依然正常闪烁
        // ★ 这就是 DMA 的价值体现：CPU 被释放了！
        LED0_TOGGLE();
        
        /* ⑨ 延时 100ms（控制 LED 闪烁速度和主循环轮询频率） */
        // 100ms 检测一次 g_adc_dma_sta 标志
        // 如果 DMA 还没采集完 50 个数据，主循环会继续执行其他任务
        delay_ms(100);
    }
}
