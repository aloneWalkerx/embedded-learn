#ifndef __SPI_H
#define __SPI_H

#include "./SYSTEM/sys/sys.h"

/* ====================================================================
   SPI1 硬件配置宏（方便移植和修改）
   ==================================================================== */

// ★ ① SPI 外设选择：使用 SPI1
// STM32F407 有 3 个 SPI（SPI1、SPI2、SPI3）
// SPI1 挂载在 APB2 总线上，时钟频率最高（84MHz）
#define SPI1_SPI                    SPI1

// ★ ② 使能 SPI1 外设时钟的宏
// 如果不使能 SPI 时钟，访问 SPI 寄存器会出错
#define SPI1_SPI_CLK_ENABLE()       do { __HAL_RCC_SPI1_CLK_ENABLE(); } while (0)

// ★ ③ SCK（时钟）引脚：PB3，复用功能 AF5（SPI1_SCK）
// SCK 由主机产生，控制通信速率
#define SPI1_SCK_GPIO_PORT          GPIOB
#define SPI1_SCK_GPIO_PIN           GPIO_PIN_3
#define SPI1_SCK_GPIO_AF            GPIO_AF5_SPI1   // AF5 = SPI1 复用功能
#define SPI1_SCK_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

// ★ ④ MISO（主入从出）引脚：PB4，复用功能 AF5（SPI1_MISO）
// MISO 负责从机 → 主机的数据传输（方向：从机输出，主机输入）
#define SPI1_MISO_GPIO_PORT         GPIOB
#define SPI1_MISO_GPIO_PIN          GPIO_PIN_4
#define SPI1_MISO_GPIO_AF            GPIO_AF5_SPI1
#define SPI1_MISO_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

// ★ ⑤ MOSI（主出从入）引脚：PB5，复用功能 AF5（SPI1_MOSI）
// MOSI 负责主机 → 从机的数据传输（方向：主机输出，从机输入）
#define SPI1_MOSI_GPIO_PORT         GPIOB
#define SPI1_MOSI_GPIO_PIN          GPIO_PIN_5
#define SPI1_MOSI_GPIO_AF            GPIO_AF5_SPI1
#define SPI1_MOSI_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

// ★ ⑥ ★ 注意：本实验没有定义 CS 片选引脚！
// 说明 CS 由用户在自己的代码中定义（因为可能接不同的引脚）

/* ====================================================================
   函数声明
   ==================================================================== */

/**
 * @brief   初始化 SPI1
 * @note    配置 SPI1 为主模式，模式 0（CPOL=0, CPHA=0）
 *          8 位数据格式，MSB 先传
 *          引脚配置为复用推挽输出模式
 * @param   无
 * @retval  无
 */
void spi1_init(void);

/**
 * @brief   设置 SPI1 通信波特率
 * @param   speed: SPI 时钟分频系数
 *          可选值：SPI_BAUDRATEPRESCALER_2 ~ 256
 *          值越小，速度越快（但需从机支持）
 *          ★ 注意：SPI1 时钟源为 84MHz
 *          例如：分频 8 → 84/8 = 10.5MHz
 * @retval  无
 */
void spi1_set_speed(uint32_t speed);

/**
 * @brief   SPI1 读写一字节数据（全双工）
 * @param   txdata: 要发送的 1 字节数据
 * @retval  接收到的 1 字节数据
 * @note    ★ SPI 全双工特性：发送的同时必定会接收！
 *          即使只读数据，也需要发送数据（通常发送 0xFF）
 *          即使只写数据，接收的数据也需忽略
 */
uint8_t spi1_read_write_byte(uint8_t txdata);

#endif
