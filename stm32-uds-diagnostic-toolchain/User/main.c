#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./SYSTEM/usart/usart.h"
#include "BSP/CAN/can.h"

int main(void)
{

    uint8_t d[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    /* ① HAL 库初始化
     * 内部做：复位所有外设、初始化 Flash 接口、配置 SysTick 为 1ms 中断
     * 【必须放最前面】—— 后面的 HAL_Delay / delay 都依赖它配好的 SysTick */
    HAL_Init();                                     // 初始化 HAL 库，配置 SysTick 为 1ms 中断

    /* ② 配置系统时钟到 168MHz
     * 参数：sys_stm32_clk_init(PLLN, PLLM, PLLP, PLLQ)，板载 HSE = 8MHz
     *
     *   算式：
     *     VCO 输入 = HSE / PLLM = 8 / 8    = 1 MHz
     *     VCO 输出 = 1MHz × PLLN = 1 × 336 = 336 MHz
     *     SYSCLK   = VCO / PLLP  = 336 / 2  = 168 MHz  ?（F407 最高主频）
     *     PLLQ 输出 = 336 / 7               = 48 MHz   ?（USB/SDIO/RNG 用）
     *
     * 分频结果（这就是"时钟树"）：
     *     AHB  = 168MHz  （GPIO 挂 AHB1）
     *     APB1 = 42MHz   （USART2/3、I2C、CAN 挂这里）
     *     APB2 = 84MHz   （USART1、SPI1 挂这里）
     *
     * ?? 进阶细节：ST 建议 VCO 输入取 2MHz（降低 PLL 抖动），这里是 1MHz
     *    在规格内（1~2MHz）但非最优。要改：sys_stm32_clk_init(168, 4, 2, 7) */
    sys_stm32_clk_init(336, 8, 2, 7);               // 配置系统时钟：PLLN=336, PLLM=8, PLLP=2, PLLQ=7，得到 168MHz

    /* ③ 初始化延时函数，传入系统时钟频率（单位 MHz）
     * 内部用 SysTick 或 TIM 实现 delay_us / delay_ms
     * 这里必须传 168 —— 传错了延时就不准 */
    delay_init(168);                                // 初始化延时函数，传入系统时钟频率 168MHz
    //初始化LED
    can_led_init();
    //初始化串口
    can_usartx_init(115200);
    //初始化CAN
    CAN2_Init(CAN_500K, 1);

    printf("CAN2 ready, loopback @500k\r\n");
    //发送一帧标准数据帧
    if (CAN2_SendStd(0x123, d, 8) == CAN_OK) {

        printf("send OK\r\n");

    }
    else {

        printf("send FAIL\r\n");

    }

    while (1) {

        delay_ms(500);

    }

}
