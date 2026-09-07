#ifndef __FLASH_H
#define __FLASH_H

#include "./SYSTEM/sys/sys.h"

/* ====================================================================
   内部 Flash 操作函数声明
   ★ 注意：STM32F407 内部 Flash 写入前必须先擦除！
   ★ 写入地址必须 4 字节对齐（32 位写入）
   ★ 操作前必须解锁 Flash，操作完成后必须上锁
   ==================================================================== */

/**
 * @brief   从指定地址读取一个字（32 位）的数据
 * @param   addr: 要读取的地址（必须 4 字节对齐）
 * @retval  读取到的 32 位数据
 * @note    内部 Flash 读取非常简单，直接通过指针访问即可
 *          不需要解锁，不需要特殊操作
 *          示例：uint32_t data = flash_read_word(0x080E0000);
 */
uint32_t flash_read_word(uint32_t addr);

/**
 * @brief   从指定地址读取指定数量的字（32 位）数据
 * @param   addr:   起始地址（必须 4 字节对齐）
 * @param   buf:    存放读取数据的缓冲区指针（uint32_t 数组）
 * @param   length: 要读取的字数（1 个字 = 4 字节）
 * @retval  无
 * @note    循环调用 flash_read_word 读取指定数量的字
 *          由于内部 Flash 总线读取速度极快，此函数执行时间很短
 *          示例：flash_read(0x080E0000, read_buf, 10);
 *               从 0x080E0000 读取 10 个字（40 字节）
 */
void flash_read(uint32_t addr, uint32_t *buf, uint32_t length);

/**
 * @brief   往指定地址不检查地写入指定数量的字（32 位）数据
 * @param   addr:   目标地址（必须 4 字节对齐）
 * @param   buf:    要写入的数据缓冲区指针（uint32_t 数组）
 * @param   length: 要写入的字数（1 个字 = 4 字节）
 * @retval  无
 * @note    ★★★ 危险函数！使用前必须确保目标地址已经被擦除！★★★
 *          此函数直接调用 HAL_FLASH_Program 写入数据，不会检查
 *          目标地址是否已擦除。如果目标地址有非 0xFF 的数据，
 *          写入会失败或数据错乱。
 *          
 *          内部流程：
 *          ① 解锁 Flash（HAL_FLASH_Unlock）
 *          ② 逐字写入（HAL_FLASH_Program）
 *          ③ 上锁 Flash（HAL_FLASH_Lock）
 *          
 *          ★ 推荐使用 stmflash_write 函数，它会自动处理擦除
 */
void flash_write_nocheck(uint32_t addr, uint32_t *buf, uint16_t length);

/**
 * @brief   往指定地址写入指定数量的字（32 位）数据（带擦除）
 * @param   addr:   目标地址（必须 4 字节对齐）
 * @param   buf:    要写入的数据缓冲区指针（uint32_t 数组）
 * @param   length: 要写入的字数（1 个字 = 4 字节）
 * @retval  无
 * @note    ★★★ 推荐使用此函数进行 Flash 写入 ★★★
 *          内部流程：
 *          ① 根据地址计算所在扇区
 *          ② 擦除整个扇区（stmflash_erase_sector）
 *          ③ 调用 stmflash_write_nocheck 写入数据
 *          
 *          示例：stmflash_write(0x080E0000, write_buf, 5);
 *               将 write_buf 中的 5 个字（20 字节）写入 0x080E0000
 *               写入前会自动擦除 0x080E0000 所在的扇区
 *          
 *          ★ 擦除扇区会清除该扇区内所有数据！
 *          确保该扇区没有需要保留的其他数据！
 */
void flash_write(uint32_t addr, uint32_t *buf, uint16_t length);

#endif
