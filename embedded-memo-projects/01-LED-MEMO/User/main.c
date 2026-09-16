/* ============================================================================
 * main.c —— 应用程序入口（Day 1：LED 跑马灯）
 * ----------------------------------------------------------------------------
 * 学习目标：脱离例程从零写出 GPIO 输出初始化，并理解 HAL → 寄存器的映射
 * ==========================================================================*/

/* 包含 sys.h：提供系统类型定义 + sys_stm32_clk_init() 时钟配置函数声明 */
#include "./SYSTEM/SYS/sys.h"

/* 包含 led.h：拿到 led_init() / LEDx_ON() / LEDx_OFF() */
#include "./BSP/LED/led.h"

/* 包含 delay.h：拿到 delay_init() / delay_ms() / delay_us() */
#include "./SYSTEM/DELAY/delay.h"


int main(void)
{
   /* ① HAL 库初始化
    * 内部做：复位所有外设、初始化 Flash 接口、配置 SysTick 为 1ms 中断
    * 【必须放最前面】—— 后面的 HAL_Delay / delay 都依赖它配好的 SysTick */
   HAL_Init();

   /* ② 配置系统时钟到 168MHz
    * 参数：sys_stm32_clk_init(PLLN, PLLM, PLLP, PLLQ)，板载 HSE = 8MHz
    *
    *   算式：
    *     VCO 输入 = HSE / PLLM = 8 / 8    = 1 MHz
    *     VCO 输出 = 1MHz × PLLN = 1 × 336 = 336 MHz
    *     SYSCLK   = VCO / PLLP  = 336 / 2  = 168 MHz  ✅（F407 最高主频）
    *     PLLQ 输出 = 336 / 7               = 48 MHz   ✅（USB/SDIO/RNG 用）
    *
    * 分频结果（这就是"时钟树"）：
    *     AHB  = 168MHz  （GPIO 挂 AHB1）
    *     APB1 = 42MHz   （USART2/3、I2C、CAN 挂这里）
    *     APB2 = 84MHz   （USART1、SPI1 挂这里）
    *
    * ⚠️ 进阶细节：ST 建议 VCO 输入取 2MHz（降低 PLL 抖动），这里是 1MHz
    *    在规格内（1~2MHz）但非最优。要改：sys_stm32_clk_init(168, 4, 2, 7) */
   sys_stm32_clk_init(336,8,2,7);

   /* ③ 初始化延时函数，传入系统时钟频率（单位 MHz）
    * 内部用 SysTick 或 TIM 实现 delay_us / delay_ms
    * 这里必须传 168 —— 传错了延时就不准 */
   delay_init(168);

   /* ④ 初始化 LED：开 GPIOF 时钟 + 配 PF9/PF10 为推挽输出 + 默认熄灭 */
   led_init();

   /* ⑤ 主循环：两个 LED 交替闪烁，各保持 500ms
    *    时序：LED0亮/LED1灭 → 500ms → LED0灭/LED1亮 → 500ms → 循环
    *    效果：两个灯交替亮，看起来像"跑"过去的灯 */
   while(1)
    {
      LED0_ON();      /* PF9 输出【低】→ LED0（红）点亮 */
      LED1_OFF();     /* PF10 输出【高】→ LED1（绿）熄灭 */
      delay_ms(500);  /* 阻塞延时 500ms */

      LED0_OFF();     /* PF9 输出【高】→ LED0 熄灭 */
      LED1_ON();      /* PF10 输出【低】→ LED1 点亮 */
      delay_ms(500);  /* 再等 500ms，形成交替节奏 */
    }

}
