/**
 ******************************************************************
 * @file        bootloader.h
 * @author      <aloneWalker>
 * @version     V1.0
 * @date        2026-11-27
 * @brief       Bootloader 跳转与 APP 合法性检查
 ******************************************************************
 * @attention
 *
 * 跳转三件套：关中断 -> 设 MSP -> 跳复位向量
 * APP 侧第一行必须重设 SCB->VTOR，否则 Boot 设的值会被 APP 启动代码覆盖
 *
 ******************************************************************
 */

#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

/* 正点原子模板总入口：uint32_t、SCB/NVIC 这些名字都从它来 */
#include "./SYSTEM/sys/sys.h"

/* ------------------------ SRAM 合法范围 ---------------------------- */
//栈顶（初始 MSP）必须落在这段里，否则 __set_MSP 之后一压栈就跑飞
#define SRAM_START_ADDR         0x20000000u
// F407 的 SRAM 上限 128KB
#define SRAM_END_ADDR           0x20020000u

/* ------------------------ APP 区长度上限 --------------------------- */
//复位向量必须小于 APP_ADDR + 960KB，防止跳到一个越界的野地址
//960KB，用于复位向量的范围检查
#define APP_MAX_SIZE            0xF0000u

/* --------------------------- 检查结果 ------------------------------ */
// 0 / 1 而不是 FLASH_OK / FLASH_ERR，因为这是"合法/不合法"，不是"成败"
#define BOOT_APP_INVALID        0
#define BOOT_APP_VALID          1

/* 函数声明 */
//检查 APP 的栈顶与复位向量是否合法
uint8_t boot_app_valid(void);
//关中断、设 MSP、跳到 APP
void boot_jump_to_app(void);

#endif
