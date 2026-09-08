#ifndef __SDCARD_H
#define __SDCARD_H

#include "./SYSTEM/sys/sys.h"


/* ====================================================================
   SDIO 引脚定义（SD 卡使用 SDIO 接口，而非 SPI）
   ★ SDIO 接口特点：
   - 4 位数据线（D0~D3），传输速度是 SPI 的 4 倍
   - 时钟频率可达 24MHz（SPI 模式通常只有 400kHz）
   - 需要配置复用功能 AF12（SDIO）
   ==================================================================== */

// ★ ① SDIO 数据线 D0（双向数据线 0）
// 用于传输数据（bit0），是 SDIO 接口的第 1 根数据线
#define SD_D0_GPIO_PORT             GPIOC
#define SD_D0_GPIO_PIN              GPIO_PIN_8    // PC8
#define SD_D0_GPIO_AF               GPIO_AF12_SDIO  // ★ 复用功能 AF12 = SDIO
#define SD_D0_GPIO_CLK_ENABLE()     do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)

// ★ ② SDIO 数据线 D1（双向数据线 1）
// 用于传输数据（bit1），4 位模式时使用
// 在 SPI 模式下可作为 MISO（主入从出）
#define SD_D1_GPIO_PORT             GPIOC
#define SD_D1_GPIO_PIN              GPIO_PIN_9    // PC9
#define SD_D1_GPIO_AF               GPIO_AF12_SDIO
#define SD_D1_GPIO_CLK_ENABLE()     do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)

// ★ ③ SDIO 数据线 D2（双向数据线 2）
// 用于传输数据（bit2），4 位模式时使用
#define SD_D2_GPIO_PORT             GPIOC
#define SD_D2_GPIO_PIN              GPIO_PIN_10   // PC10
#define SD_D2_GPIO_AF               GPIO_AF12_SDIO
#define SD_D2_GPIO_CLK_ENABLE()     do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)

// ★ ④ SDIO 数据线 D3（双向数据线 3）
// 用于传输数据（bit3），4 位模式时使用
// ★ 注意：D3 在 SD 卡初始化阶段也用作 CS（片选）信号
#define SD_D3_GPIO_PORT             GPIOC
#define SD_D3_GPIO_PIN              GPIO_PIN_11   // PC11
#define SD_D3_GPIO_AF               GPIO_AF12_SDIO
#define SD_D3_GPIO_CLK_ENABLE()     do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)

// ★ ⑤ SDIO 时钟线 SCK
// 由主机产生，控制 SD 卡通信速率
// SDIO 模式下频率可达 24MHz（比 SPI 快得多）
#define SD_SCK_GPIO_PORT            GPIOC
#define SD_SCK_GPIO_PIN             GPIO_PIN_12   // PC12
#define SD_SCK_GPIO_AF              GPIO_AF12_SDIO
#define SD_SCK_GPIO_CLK_ENABLE()    do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)

// ★ ⑥ SDIO 命令线 CMD
// 用于发送命令和接收响应（双向）
// 主机通过 CMD 线发送命令，SD 卡通过 CMD 线返回响应
#define SD_CMD_GPIO_PORT            GPIOD
#define SD_CMD_GPIO_PIN             GPIO_PIN_2    // PD2
#define SD_CMD_GPIO_AF              GPIO_AF12_SDIO
#define SD_CMD_GPIO_CLK_ENABLE()    do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

/* ====================================================================
   SDIO 接口说明：
   ┌─────────────────────────────────────────────────────────────────────────────┐
   │  SDIO 引脚  │  STM32 引脚  │  方向    │  作用                             │
   │  ───────────┼──────────────┼──────────┼───────────────────────────────── │
   │  D0         │  PC8         │  双向    │  数据线 0（bit0）                 │
   │  D1         │  PC9         │  双向    │  数据线 1（bit1）                 │
   │  D2         │  PC10        │  双向    │  数据线 2（bit2）                 │
   │  D3         │  PC11        │  双向    │  数据线 3（bit3）/ 初始化 CS      │
   │  SCK        │  PC12        │  主机→卡 │  时钟信号（最高 24MHz）           │
   │  CMD        │  PD2         │  双向    │  命令/响应线                      │
   └─────────────────────────────────────────────────────────────────────────────┘
   ==================================================================== */

/* ====================================================================
   SD 卡操作超时时间定义
   ==================================================================== */

// ★ ⑦ SD 卡操作超时时间（约 100ms）
// 用于等待 SD 卡响应或操作完成
// 如果 SD 卡在 100ms 内没有响应，认为操作失败
#define SD_DATATIMEOUT              ((uint32_t)100000000)

/* ====================================================================
   外部变量声明
   ==================================================================== */

// ★ ⑧ SD 卡句柄（HAL 库用于管理 SDIO 外设）
// 在 sdcard.c 中定义，外部文件可通过 extern 引用
extern SD_HandleTypeDef g_sd_handle;

// ★ ⑨ SD 卡信息结构体（存储卡容量、类型、块大小等）
// 在 sdcard.c 中定义，外部文件可通过 extern 引用
extern HAL_SD_CardInfoTypeDef g_sd_card_info;

/* ====================================================================
   函数声明
   ==================================================================== */

/**
 * @brief   初始化 SD 卡
 * @param   无
 * @retval  初始化结果
 * @arg     0: 成功
 * @arg     1: 失败
 * @note    内部流程：
 *          ① 初始化 SDIO 引脚（D0~D3、SCK、CMD）
 *          ② 配置 SDIO 时钟（频率、模式）
 *          ③ 发送命令识别 SD 卡
 *          ④ 获取 SD 卡信息（容量、类型等）
 *          ⑤ 切换到 4 位数据模式
 */
uint8_t sd_init(void);

/**
 * @brief   读 SD 卡指定数量的块数据
 * @param   buf:    存储读取数据的缓冲区指针
 * @param   addr:   起始块地址（扇区号）
 * @param   count:  要读取的块数量（1 块 = 512 字节）
 * @retval  读取结果
 * @arg     0: 成功
 * @arg     1: 失败
 * @note    使用 HAL_SD_ReadBlocks 读取数据
 *          ★ 地址单位是“块”，不是“字节”
 *          例如：addr=0 表示第 0 个块（MBR），addr=1 表示第 1 个块
 *          块大小通常为 512 字节
 */
uint8_t sd_read_disk(uint8_t *buf, uint32_t addr, uint32_t count);

/**
 * @brief   写 SD 卡指定数量的块数据
 * @param   buf:    要写入的数据缓冲区指针
 * @param   addr:   起始块地址（扇区号）
 * @param   count:  要写入的块数量（1 块 = 512 字节）
 * @retval  写入结果
 * @arg     0: 成功
 * @arg     1: 失败
 * @note    使用 HAL_SD_WriteBlocks 写入数据
 *          ★ 写入前需确保 SD 卡未被写保护
 *          ★ 块大小通常为 512 字节
 */
uint8_t sd_write_disk(uint8_t *buf, uint32_t addr, uint32_t count);

#endif
