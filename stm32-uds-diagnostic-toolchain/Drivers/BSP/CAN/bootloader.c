/**
 ******************************************************************
 * @file        bootloader.c
 * @author      <aloneWalker>
 * @version     V1.0
 * @date        2026-11-27
 * @brief       Bootloader 跳转与 APP 合法性检查
 ******************************************************************
 * @attention
 *
 * 实验平台:正点原子 M144Z-M4最小系统板STM32F407
 * 本文件遵循正点原子例程的文件格式：宏在 .h，.c 只使用宏
 *
 ******************************************************************
 */
#include "./BSP/CAN/bootloader.h"
#include "./BSP/CAN/diag_cfg.h"

//函数指针类型：把复位向量当地址调
typedef void(*pFunc) (void);

//简单合法性检查：栈顶在 SRAM、复位向量在 APP 区
/**
 * @brief  检查 APP 区是否有效（是否包含合法的固件）
 * @return 1 = 有效，可以跳转；0 = 无效，不能跳转
 * @note   STM32 的 APP 固件开头前 8 个字节有固定格式：
 *         第 1 个 4 字节：栈顶地址（MSP）
 *         第 2 个 4 字节：复位中断函数入口地址（Reset_Handler）
 *         如果这两个值合理，说明 APP 是完整的，可以安全跳过去。
 */
uint8_t boot_app_valid(void)
{
    /* APP 的第 1 个字 = 初始 MSP（栈顶），第 2 个字 = 复位向量。
       volatile 不能省：不加的话编译器可能把这次读优化掉或缓存住 */
    uint32_t sp = * (volatile uint32_t *)APP_ADDR;  // 读 APP 区的第 1 个字（栈顶地址）
    uint32_t rv = * (volatile uint32_t *) (APP_ADDR +4u); // 读 APP 区的第 2 个字（复位向量）

    // 栈顶必须落在 SRAM 里，否则 __set_MSP 之后第一次压栈就跑飞
    // （SRAM 范围通常是 0x20000000 ~ 0x2001FFFF，如果不在这范围，说明 APP 是空的或者是乱码）
    if (sp < SRAM_START_ADDR || sp > SRAM_END_ADDR)return 0;

    // 复位向量必须在 APP 区内：低于 APP_ADDR 说明是空片或指回 BOOT，
    // 超过 960KB 说明越界 —— 这两种情况都不许跳
    if (rv < APP_ADDR || rv >= (APP_ADDR +APP_MAX_SIZE))return 0;

    /* 最低位是 Thumb 位：Cortex-M 只跑 Thumb 指令，这一位为 0 就不是
       合法入口，跳过去立刻 HardFault（擦干净的 Flash 是 0xFF，所以空片
       这里也过不了） */
    if ((rv & 1u) == 0)return 0;                    // 检查最低位，必须是 1

    return 1;                                       // 三项检查全部通过，APP 有效
}

/**
 * @brief  从 BOOT 跳转到 APP 程序
 * @note   这是整个 Bootloader 最后一步。跳过去之后，BOOT 就彻底“退休”了。
 *         跳转前要做一系列清理工作，保证 APP 能干净地接手。
 *         如果 APP 不合法，直接返回，BOOT 继续在 while(1) 里等刷写。
 */
void boot_jump_to_app(void)
{
    uint32_t sp, rv;
    uint8_t i;

    /* 不合法就原地不动，回主循环继续等刷写 */
    if (!boot_app_valid())return;

    // 重新读一遍，再确认一次（防止刚好在检查完这瞬间，有人通过刷写把 APP 改坏了）
    sp = * (volatile uint32_t *)APP_ADDR;           // 重新读栈顶地址
    rv = * (volatile uint32_t *) (APP_ADDR +4u);    // 重新读复位向量

    // 第一件事：关全局中断 PRIMASK=1
    // 意思是从现在起，任何中断（CAN接收、串口、定时器）都不会响应
    __disable_irq();

    /* 注意：全局中断从这里开始是关着的，
       APP 的 main() 里必须调用 __enable_irq() 重新打开，
       否则 APP 里 CAN 接收中断、SysTick 都不会触发
       （串口轮询打印正常，容易误判为一切正常） */

    /* 关掉所有外设中断，清掉 SysTick。
       F407 有 82 个中断，ICER 至少 3 个寄存器；写 8 个把全部覆盖掉，
       多写的部分硬件忽略，不会出错 */
    for (i = 0; i < 8; i++)NVIC->ICER[i] = 0xFFFFFFFFu; // 关闭所有 NVIC 中断使能

    // 关掉 SysTick 计数器和中断（否则跳过去后 SysTick 还在跑，可能会触发中断打到 BOOT 的代码段）
    SysTick->CTRL = 0;                              // 关总开关
    // 重装值清零
    SysTick->LOAD = 0;
    // 当前值清零，避免带着旧计数跳过去
    SysTick->VAL = 0;

    // 向量表搬到 0x08010000，APP 里还会再设一次
    // 为什么必须设？因为 APP 里的中断函数地址是基于 APP_ADDR 的，不搬过去中断会乱飞
    SCB->VTOR = APP_ADDR;

    // 换栈：不换的话 APP 用的还是 BOOT 的栈，可能会把 BOOT 的栈变量覆盖掉
    __set_MSP(sp);

    // 跳复位向量 = 直接进 APP 的 Reset_Handler
    // 这行代码一执行，就相当于给 APP 按了“开机键”，BOOT 从此结束使命
    ((pFunc)rv) ();

    // 正常回不来；真回来也别乱跑，停这儿
    while (1) {}
}
