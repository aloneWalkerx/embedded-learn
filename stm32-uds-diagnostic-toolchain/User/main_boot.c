/**
 ******************************************************************
 * @file        main_boot.c
 * @author      <aloneWalker>
 * @version     V1.0
 * @date        2026-11-27
 * @brief       Bootloader 主程序
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
#include "./BSP/CAN/bootloader.h"
#include "./BSP/CAN/diag_cfg.h"

/* 两个 main 都留在工程里，靠这个宏决定谁参与编译：
   BOOT Target 的 C/C++ Define 里必须有 TARGET_BOOT */
#if defined(TARGET_BOOT)
int main(void)
{
    //窗口起点（毫秒 tick）
    uint32_t t0;
    //复位外设 + 配好 1ms 的 SysTick
    HAL_Init();
    //正点原子 HAL 模板：168MHz
    sys_stm32_clk_init(336, 8, 2, 7);
    //告诉 delay 模块主频 168MHz，算循环次数
    delay_init(168);
    //串口 115200，调试打印用
    can_usartx_init(115200);

    printf("\r\n=== BOOT v1.0 ===\r\n");
    //1 = APP 头合法
    printf("APP valid = %d\r\n", boot_app_valid());
    //正常模式
    CAN2_Init(CAN_500K, 0);
    //只收 0x7E0 / 0x7DF
    CAN2_Filter_List2(ISO_RX_ID, ISO_FUNC_ID);
    CAN2_IRQ_Enable();
    //收 0x7E0，发 0x7E8
    isotp_init(ISO_RX_ID, ISO_TX_ID);
    uds_init();

    // 2 秒窗口：有人发 10 02 就不跳
    t0 = HAL_GetTick();
    // 用无符号减法算时间差：tick 计满 49 天会回绕，这样写回绕也正确
    while ((uint32_t) (HAL_GetTick() - t0) < 2000u) {
        //收请求、回响应，都在这里发生
        uds_poll();
    }

    // 两个条件都满足才跳：APP 头合法 + 没人在诊断会话里（即没发 10 02）
    if (boot_app_valid() && uds_session() == SESS_DEFAULT) {
        printf("jump to APP @0x%08X\r\n", APP_ADDR);
        //等串口把上面这行发完，跳过去 APP 会重配串口
        delay_ms(20);
        boot_jump_to_app();
    }

    printf("stay in BOOT, waiting for download...\r\n");
    //留在 BOOT：擦写跳都在这儿做
    while (1) {
        uds_poll();
    }
}
#endif
