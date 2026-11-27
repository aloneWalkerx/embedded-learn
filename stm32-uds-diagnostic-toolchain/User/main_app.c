/**
 ******************************************************************
 * @file        main_app.c
 * @author      <aloneWalker>
 * @version     V1.0
 * @date        2026-11-27
 * @brief       APP 主程序
 ******************************************************************
 * @attention
 *
 * 实验平台:正点原子 M144Z-M4最小系统板STM32F407
 * 本文件遵循正点原子例程的文件格式：宏在 .h，.c 只使用宏
 *
 ******************************************************************
 */
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/CAN/can.h"
#include "./BSP/CAN/isotp.h"
#include "./BSP/CAN/uds.h"
#include "./BSP/CAN/diag_cfg.h"

/* 只编 TARGET_APP 这个 Target 时这个 main 才生效 */
#if defined(TARGET_APP)
int main(void)
{
    HAL_Init();                                     /* HAL 初始化 + 1ms SysTick */
    sys_stm32_clk_init(336, 8, 2, 7);               /* 正点原子 HAL 模板：168MHz */
    delay_init(168);
    can_usartx_init(115200);

    /* ===== 这一行是 APP 的第一行，少它必死 ===== */
    SCB->VTOR = APP_ADDR;                           /* 向量表基址 = 0x08010000，中断才找得到入口 */

    /* 关键：BOOT 跳转前执行了 __disable_irq()，全局中断是关着的。
       APP 不重新打开的话，串口能打印（轮询），但 CAN 接收中断一次都不会触发。 */
    __enable_irq();                                 /* 清 PRIMASK，把全局中断放开 */

    printf("\r\n=== APP %s ===\r\n", APP_VERSION);
    printf("VTOR = 0x%08X\r\n", SCB->VTOR);         /* 打出来确认真的设上了 */

    CAN2_Init(CAN_500K, 0);
    CAN2_Filter_List2(ISO_RX_ID, ISO_FUNC_ID);
    CAN2_IRQ_Enable();
    isotp_init(ISO_RX_ID, ISO_TX_ID);
    uds_init();

    while (1) {
        uds_poll();                                 /* S3 超时、挂起计时等都在这 */
    }
}
#endif                                              /* TARGET_APP */
