#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/CAN/can_test.h"
#include "./BSP/CAN/can2.h"
#include "./BSP/CAN/isotp.h"
#include "./BSP/CAN/diag_cfg.h"
#include "./BSP/CAN/common.h"
int main(void)
{
    uint8_t test[20];
    uint8_t i;

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
    //环回自测
    //can_test_loopback();

    //正常模式
    //can_test_normal();
    //过滤指定ID
    //can_filter_list2();
    //can_node_test();

    CAN2_Init(CAN_500K, 0);
    CAN2_Filter_List2(ISO_RX_ID, ISO_FUNC_ID);
    CAN2_IRQ_Enable();
    isotp_init(ISO_RX_ID, ISO_TX_ID);
    while (1) {
        /* 每秒发一条 20 字节测试报文（首帧 + 连续帧） */
        for (i = 0; i < 20; i++) {
            test[i] = (uint8_t)i;
        }
        printf("send 20B -> %s\r\n",
        (isotp_send(test, 20) == 0) ? "ok" : "fail");

        if (g_iso_req_pending) {
            g_iso_req_pending = 0;
            printf("rx complete!\r\n");
        }
        delay_ms(1000);
    }

}
