/**
 ******************************************************************
 * @file        flash.h
 * @author      <aloneWalker>
 * @version     V1.0
 * @date        2026-11-26
 * @brief       STM32F407 内部 Flash 驱动（APP 区）+ CRC32
 ******************************************************************
 * @attention
 *
 * APP 区：扇区 4~11，0x08010000 起，共 960KB
 * 注意：扇区 4 是 64KB，扇区 5~11 是 128KB —— 擦除循环不能假设等大
 * 编程单位是字（4 字节），非 4 字节对齐的尾包要先补 0xFF
 *
 ******************************************************************
 */

#ifndef __FLASH_H
#define __FLASH_H

#include "./SYSTEM/sys/sys.h"

/* ------------------------- 解锁魔法数 ------------------------------- */
//为什么要"魔法数"：防止程序跑飞时误改 Flash。KEYR 只要收到一个错值就把 LOCK 位置 1，而且本次上电内不再接受解锁，只能复位重来
#ifndef FLASH_KEY1
//第一步：写 KEY1
#define FLASH_KEY1              0x45670123u
#endif
#ifndef FLASH_KEY2
//第二步：写 KEY2，两步都对才解锁
#define FLASH_KEY2              0xCDEF89ABu
#endif

/* -------------------------- 编程宽度 -------------------------------- */
/* PSIZE 是 FLASH->CR 的 [2:1] 位：00=x8、01=x16、10=x32、11=x64。
   x32 只在 2.7~3.6V 下允许，所以同时要配 VOLTAGE_RANGE_3；
   电压不够还按 x32 写，数据会写坏 */
//2 = x32，3.3V 下允许、擦写最快
#define FLASH_PSIZE_X32         2
//告诉 HAL 用 3.3V 档
#define FLASH_VRANGE_3V3        FLASH_VOLTAGE_RANGE_3

/* -------------------------- CRC32 ---------------------------------- */
/* CRC32 有五项参数：多项式 / 初值 / 位序(MSB first) / 反射(无) / 输出异或(无)。
   五项里错一项，两端的值就完全对不上，所以这里把它单列出来 */
//多项式（与 PC 端一致）
#define CRC_POLY                0x04C11DB7u
//初值：全 1，不是 0
#define CRC_INIT_VAL            0xFFFFFFFFu

/* ------------------------- 唯一 ID 地址 ---------------------------- */
/* 0x1FFF7A10 在系统存储区，出厂烧好、普通擦除动不到；共 96 位 = 3 个字 */
#define UID_BASE_ADDR           0x1FFF7A10u
//3 个 32 位字 = 12 字节
#define UID_WORD_NUM            3

/* --------------------------- 返回值 -------------------------------- */
//0 = 成功（和 HAL_OK 同值）
#define FLASH_OK                0
//1 = 失败，调用方只判 != FLASH_OK
#define FLASH_ERR               1

/* 固件长度与 CRC，由 uds_flash.c 在下载过程中维护 */
//有效固件字节数（不含末尾 4 字节 CRC）
extern uint32_t g_fw_len;
//PC 算好、追加在镜像末尾的 CRC32
extern uint32_t g_fw_crc;

/* 函数声明：这里只声明，实现全在 flash.c */
//解锁，返回 FLASH_OK 才能擦写
uint8_t flash_unlock(void);
//上锁，防跑飞误写
void flash_lock(void);
//擦一个扇区，sector = 0~11
uint8_t flash_erase_sector(uint32_t sector);
//擦扇区 4~11，最坏约 15 秒
uint8_t flash_erase_app(void);
/* flash_program：从 addr 起逐字写 len 字节，len 必须是 4 的倍数 */
uint8_t flash_program(uint32_t addr, uint8_t *buf, uint32_t len);
//CRC 校验，0 = 通过
uint8_t flash_check_app(void);
//读 96 位 UID 到 out12[0..11]
void flash_read_uid(uint8_t *out12);
#endif
