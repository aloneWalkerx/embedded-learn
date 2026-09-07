#ifndef __NORFLASH_H
#define __NORFLASH_H

#include "./SYSTEM/sys/sys.h"

/* ====================================================================
   NOR Flash CS（片选）引脚定义
   ★ CS 引脚控制 Flash 是否被选中（低电平有效）
   ==================================================================== */

// ★ ① CS 引脚端口：GPIOB
#define NORFLASH_CS_GPIO_PORT           GPIOB

// ★ ② CS 引脚编号：Pin 14（PB14）
#define NORFLASH_CS_GPIO_PIN            GPIO_PIN_14

// ★ ③ 使能 GPIOB 时钟的宏
#define NORFLASH_CS_GPIO_CLK_ENABLE()   do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

/* ====================================================================
   CS 片选控制宏（低电平有效）
   ==================================================================== */

// ★ ④ CS 控制宏
// 用法：NORFLASH_CS(0) → 选中 Flash（CS 拉低）
//      NORFLASH_CS(1) → 释放 Flash（CS 拉高）
// ★ 注意：SPI 通信前必须先 CS=0，通信结束后 CS=1
#define NORFLASH_CS(x)                  do { (x) ?                                                                          \
                                            HAL_GPIO_WritePin(NORFLASH_CS_GPIO_PORT, NORFLASH_CS_GPIO_PIN, GPIO_PIN_SET):   \
                                            HAL_GPIO_WritePin(NORFLASH_CS_GPIO_PORT, NORFLASH_CS_GPIO_PIN, GPIO_PIN_RESET); \
                                        } while (0)

/* ====================================================================
   NOR Flash 芯片 ID 定义
   格式：0xXXYY → XX=厂商ID，YY=容量ID
   ★ 用于识别不同型号的 Flash 芯片
   ==================================================================== */

#define W25Q80                          0xEF13   // Winbond 8Mbit (1MB)
#define W25Q16                          0xEF14   // Winbond 16Mbit (2MB)
#define W25Q32                          0xEF15   // Winbond 32Mbit (4MB)
#define W25Q64                          0xEF16   // Winbond 64Mbit (8MB)
#define W25Q128                         0xEF17   // Winbond 128Mbit (16MB) ★ 本实验使用
#define W25Q256                         0xEF18   // Winbond 256Mbit (32MB)
#define BY25Q64                         0x6816   // Boya 64Mbit (8MB)
#define BY25Q128                        0x6817   // Boya 128Mbit (16MB)
#define NM25Q64                         0x5216   // Numonyx 64Mbit (8MB)
#define NM25Q128                        0x5217   // Numonyx 128Mbit (16MB)

/* ====================================================================
   NOR Flash 指令码定义
   ★ 这些指令通过 MOSI 线发送给 Flash，控制 Flash 执行各种操作
   ==================================================================== */

// ★ 写使能/禁止（写入前必须先发写使能）
#define NORFLASH_WriteEnable            0x06    // 写使能（必须！）
#define NORFLASH_WriteDisable           0x04    // 写禁止

// ★ 状态寄存器操作（用于查询忙状态、保护状态等）
#define NORFLASH_ReadStatusReg1         0x05    // 读状态寄存器 1（bit0 = BUSY 位）
#define NORFLASH_ReadStatusReg2         0x35    // 读状态寄存器 2
#define NORFLASH_ReadStatusReg3         0x15    // 读状态寄存器 3
#define NORFLASH_WriteStatusReg1        0x01    // 写状态寄存器 1
#define NORFLASH_WriteStatusReg2        0x31    // 写状态寄存器 2
#define NORFLASH_WriteStatusReg3        0x11    // 写状态寄存器 3

// ★ 读取数据
#define NORFLASH_ReadData               0x03    // 标准读（发送地址后连续读）
#define NORFLASH_FastReadData           0x0B    // 快速读（带 dummy 字节）
#define NORFLASH_FastReadDual           0x3B    // 双线快速读
#define NORFLASH_FastReadQuad           0xEB    // 四线快速读

// ★ 写入数据
#define NORFLASH_PageProgram            0x02    // 页编程（一次最多 256 字节）
#define NORFLASH_PageProgramQuad        0x32    // 四线页编程

// ★ 擦除操作（★ 写入前必须先擦除！）
#define NORFLASH_BlockErase             0xD8    // 块擦除（64KB）
#define NORFLASH_SectorErase            0x20    // ★ 扇区擦除（4KB，最常用）
#define NORFLASH_ChipErase              0xC7    // 全片擦除（耗时较长）

// ★ 电源管理
#define NORFLASH_PowerDown              0xB9    // 进入掉电模式（省电）
#define NORFLASH_ReleasePowerDown       0xAB    // 退出掉电模式（恢复）

// ★ 读取 ID
#define NORFLASH_DeviceID               0xAB    // 读取设备 ID
#define NORFLASH_ManufactDeviceID       0x90    // 读取制造商/设备 ID
#define NORFLASH_JedecDeviceID          0x9F    // ★ 读取 JEDEC ID（本实验使用）

// ★ 4 字节地址模式（用于 >16MB 的芯片）
#define NORFLASH_Enable4ByteAddr        0xB7    // 使能 4 字节地址模式
#define NORFLASH_Exit4ByteAddr          0xE9    // 退出 4 字节地址模式

// ★ 其他
#define NORFLASH_SetReadParam           0xC0    // 设置读取参数
#define NORFLASH_EnterQPIMode           0x38    // 进入 QPI 模式
#define NORFLASH_ExitQPIMode            0xFF    // 退出 QPI 模式

/* ====================================================================
   函数声明
   ==================================================================== */

/**
 * @brief   初始化 NOR Flash
 * @note    1. 初始化 SPI1（调用 spi1_init）
 *          2. 配置 CS 引脚为推挽输出
 *          3. 默认 CS 拉高（不选中）
 * @param   无
 * @retval  无
 */
void norflash_init(void);

/**
 * @brief   写使能 NOR Flash
 * @note    发送 0x06 指令，使 Flash 允许写入
 *          ★ 执行写/擦除操作前必须先调用此函数！
 * @param   无
 * @retval  无
 */
void norflash_write_enable(void);

/**
 * @brief   读 NOR Flash 的状态寄存器
 * @param   regno: 状态寄存器编号（1、2、3）
 * @retval  状态寄存器值
 * @note    regno=1 时读状态寄存器 1（bit0 = BUSY 位）
 *          常用于检测 Flash 是否忙
 */
uint8_t norflash_read_sr(uint8_t regno);

/**
 * @brief   写 NOR Flash 的状态寄存器
 * @param   regno: 状态寄存器编号（1、2、3）
 * @param   sr:    要写入的值
 * @retval  无
 * @note    用于配置 Flash 的保护状态等
 */
void norflash_write_sr(uint8_t regno, uint8_t sr);

/**
 * @brief   读 NOR Flash 芯片 ID
 * @param   无
 * @retval  16 位芯片 ID
 * @note    发送 0x9F 指令，读取 3 个字节的 JEDEC ID
 *          W25Q128 返回 0xEF17（EF=Winbond，17=128Mbit）
 */
uint16_t norflash_read_id(void);

/**
 * @brief   从 NOR Flash 读取数据
 * @param   pbuf:   存放读取数据的缓冲区指针
 * @param   addr:   起始地址（24 位，0~0xFFFFFF）
 * @param   datalen: 要读取的数据长度（字节）
 * @retval  无
 * @note    发送 0x03 指令 + 24 位地址
 *          然后连续读取 datalen 个字节
 */
void norflash_read(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/**
 * @brief   向 NOR Flash 写入数据
 * @param   pbuf:   要写入的数据缓冲区指针
 * @param   addr:   起始地址（24 位，0~0xFFFFFF）
 * @param   datalen: 要写入的数据长度（字节）
 * @retval  无
 * @note    1. 写入前必须先擦除对应扇区！
 *          2. 每页最多 256 字节，函数内部自动处理跨页
 *          3. 写入前自动发送写使能
 *          4. 写入后等待 BUSY 位清除
 */
void norflash_write(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/**
 * @brief   擦除整个 NOR Flash 芯片
 * @param   无
 * @retval  无
 * @note    发送 0xC7 指令
 *          ★ 耗时较长（约 30~50 秒）
 *          ★ 擦除后所有数据变为 0xFF
 */
void norflash_erase_chip(void);

/**
 * @brief   擦除 NOR Flash 一个扇区（4KB）
 * @param   saddr: 扇区地址（任意扇区内地址即可）
 * @retval  无
 * @note    发送 0x20 指令 + 24 位地址
 *          ★ 耗时约 30~50ms
 *          ★ 擦除后扇区内所有数据变为 0xFF
 *          擦除是写入的前提！
 */
void norflash_erase_sector(uint32_t saddr);

#endif

