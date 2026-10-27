/**
 ****************************************************************************************************
 * @file        diag_cfg.h
 * @author      <aloneWalker>
 * @version     V1.0
 * @date        2026-10-27
 * @brief       全项目唯一的配置入口（ID / 时间 / 分区 / 版本）
 ****************************************************************************************************
 * @attention
 *
 * 任何 ID、超时、常量只在这里改，别的文件不许写死数字。
 * Flash 分区：BOOT 扇区 0~3（64KB）/ APP 扇区 4~11（960KB）
 *
 ****************************************************************************************************
 */

#ifndef __CAN_DIAG_CFG_H
#define __CAN_DIAG_CFG_H

#include "./SYSTEM/sys/sys.h"

/* ---------------------------- 诊断 ID ------------------------------- */
//ECU 收请求（PC -> ECU）
#define ISO_RX_ID               0x7E0u
//ECU 发响应（ECU -> PC）
#define ISO_TX_ID               0x7E8u
//功能寻址（广播）
#define ISO_FUNC_ID             0x7DFu

/* ---------------------------- ISO-TP -------------------------------- */
//ISO 15765-2 上限
#define ISO_MAX_LEN             4095
//本项目 CF 间隔
#define ISO_STMIN_MS            5
//我方回 FC 时通告 BS=0（不用再流控）
#define ISO_BS_ADVERT           0
//等对端 FC 的超时 ms
#define ISO_FC_TIMEOUT          200

/* -------------------------- UDS 时间参数 ---------------------------- */
#define UDS_P2_MS               50u
#define UDS_P2STAR_MS           5000u
#define UDS_S3_MS               5000u

/* --------------------------- Flash 分区 ----------------------------- */
//扇区 0~3，64KB
#define BOOT_ADDR               0x08000000u
//扇区 4 起，960KB
#define APP_ADDR                0x08010000u
#define APP_SECTOR_FIRST        4
#define APP_SECTOR_LAST         11
#define FLASH_SECTOR_NUM        12
//APP 区最大 1MB，用于地址越界检查
#define APP_SIZE_MAX            0x100000u

/* --------------------------- 应用信息 ------------------------------- */
#define APP_VERSION             "V1.0.0"
#define VIN_STR                 "LSVAA1234567890AB"

/* ------------------------ 安全访问参数 ------------------------------ */
//种子/密钥长度（字节）
#define SEC_SEED_LEN            4
//连续错误次数上限
#define SEC_MAX_ATTEMPTS        3
//超限后锁定时长
#define SEC_LOCK_MS             10000
//密钥算法：异或常量
#define SEC_KEY_XOR             0xA5A5A5A5u
//密钥算法：循环左移位数
#define SEC_KEY_ROT             3
//密钥算法：相加常量
#define SEC_KEY_ADD             0x1A2B3C4Du

/* ------------------------ 刷写（0x34/36/37）------------------------ */
//高 4 位=长度字节数 / 低 4 位=地址字节数
#define DL_ALFID                0x44
#define DL_ADDR_LEN             4
#define DL_SIZE_LEN             4
//每块数据上限（应答里报 512+2）
#define DL_MAX_BLOCK_DATA       512
#define DL_BLOCK_LEN            (DL_MAX_BLOCK_DATA + 2)

/* --------------------------- 其它 ----------------------------------- */
//毫秒时基
#define DIAG_TICK()             HAL_GetTick()

#endif
