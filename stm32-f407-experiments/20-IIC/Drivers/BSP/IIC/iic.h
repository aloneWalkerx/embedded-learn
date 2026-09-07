#ifndef __IIC_H
#define __IIC_H

#include "./SYSTEM/sys/sys.h"

/* ====================================================================
   引脚定义（使用 GPIOB 的 PB8（SCL）和 PB9（SDA））
   ==================================================================== */

// ★ ① SCL 时钟线引脚端口：GPIOB
// SCL 负责传输时钟信号，由主机（STM32）产生
#define IIC_SCL_GPIO_PORT           GPIOB

// ★ ② SCL 时钟线引脚编号：Pin 8（PB8）
#define IIC_SCL_GPIO_PIN            GPIO_PIN_8

// ★ ③ 使能 GPIOB 时钟的宏（SCL 和 SDA 共用 GPIOB）
// 必须使能 GPIO 时钟，否则无法控制引脚电平
#define IIC_SCL_GPIO_CLK_ENABLE()   do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

// ★ ④ SDA 数据线引脚端口：GPIOB
// SDA 负责传输数据，是双向引脚（主机和从机都可控制）
#define IIC_SDA_GPIO_PORT           GPIOB

// ★ ⑤ SDA 数据线引脚编号：Pin 9（PB9）
#define IIC_SDA_GPIO_PIN            GPIO_PIN_9

// ★ ⑥ 使能 GPIOB 时钟的宏（SCL 和 SDA 共用 GPIOB）
#define IIC_SDA_GPIO_CLK_ENABLE()   do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

/* ====================================================================
   GPIO 操作宏（控制 SCL 和 SDA 的电平）
   ==================================================================== */

// ★ ⑦ 控制 SCL 时钟线电平的宏
// 用法：IIC_SCL(1) → 拉高 SCL，IIC_SCL(0) → 拉低 SCL
// 参数 x：1 表示高电平，0 表示低电平
// ★ 注意：此宏使用 do...while(0) 包装，确保在任何地方都能安全使用
#define IIC_SCL(x)                  do { (x) ?                                                                  \
                                        HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_SET):   \
                                        HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_RESET); \
                                    } while (0)

// ★ ⑧ 控制 SDA 数据线电平的宏
// 用法：IIC_SDA(1) → 拉高 SDA，IIC_SDA(0) → 拉低 SDA
// 参数 x：1 表示高电平，0 表示低电平
// ★ 注意：当主机发送数据时，SDA 由主机控制；读取数据时，SDA 由从机控制
#define IIC_SDA(x)                  do { (x) ?                                                                  \
                                        HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_SET):   \
                                        HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_RESET); \
                                    } while (0)


// ★ ⑨ 读取 SDA 数据线当前电平的宏
// 用法：uint8_t level = IIC_SDA_READ;   // level = 0 或 1
// ★ 注意：读取 SDA 之前，必须先将 SDA 引脚切换为输入模式！
//         否则无法读取从机发送的数据（如应答信号）
#define IIC_SDA_READ                ((HAL_GPIO_ReadPin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN) == GPIO_PIN_RESET) ? 0 : 1)


/* ====================================================================
   函数声明（模拟 IIC 时序的核心函数）
   ==================================================================== */

/**
 * @brief   初始化 IIC（模拟 IIC）
 * @note    配置 SCL 和 SDA 引脚为开漏输出模式
 *          ★ IIC 协议要求 SDA 和 SCL 必须为开漏模式
 *          ★ 外部必须接上拉电阻（4.7kΩ ~ 10kΩ）
 *          开漏模式 + 上拉电阻实现了 "线与" 功能
 * @param   无
 * @retval  无
 */
void iic_init(void);

/**
 * @brief   产生 IIC 起始信号（START）
 * @note    时序要求：SCL=1 时，SDA 从 1→0 跳变
 *          起始信号后，总线被主机占用，从机开始监听
 * @param   无
 * @retval  无
 */
void iic_start(void);

/**
 * @brief   产生 IIC 停止信号（STOP）
 * @note    时序要求：SCL=1 时，SDA 从 0→1 跳变
 *          停止信号后，总线释放，从机停止监听
 * @param   无
 * @retval  无
 */
void iic_stop(void);

/**
 * @brief   等待 IIC 应答信号（ACK）
 * @note    主机发送完一个字节后，释放 SDA
 *          从机在第 9 个时钟周期将 SDA 拉低，表示“收到”
 *          返回值：0=收到 ACK（成功），1=未收到 ACK（失败）
 * @param   无
 * @retval  0：收到 ACK，1：未收到 ACK
 */
uint8_t iic_wait_ack(void);

/**
 * @brief   产生 IIC ACK 信号（应答）
 * @note    主机在接收完数据后，拉低 SDA 表示“收到”
 *          告诉从机：继续发送下一个字节
 * @param   无
 * @retval  无
 */
void iic_ack(void);

/**
 * @brief   产生 IIC NACK 信号（非应答）
 * @note    主机在接收完数据后，保持 SDA 高电平
 *          告诉从机：不要再发了，停止传输
 * @param   无
 * @retval  无
 */
void iic_nack(void);

/**
 * @brief   IIC 发送一个字节（8 位）
 * @param   data: 要发送的数据（1 字节）
 * @note    从最高位（bit7）开始发送，逐位送出
 *          发送完成后，等待从机应答（由调用者处理）
 * @retval  无
 */
void iic_send_byte(uint8_t data);

/**
 * @brief   IIC 读取一个字节（8 位）
 * @param   ack: 读取完成后是否发送 ACK
 *          1：发送 ACK（继续接收），0：发送 NACK（停止接收）
 * @note    从最高位（bit7）开始读取，逐位读入
 *          ★ 读取前必须将 SDA 切换为输入模式
 *          ★ 读取后根据 ack 参数决定是否应答
 * @retval  读取到的数据（1 字节）
 */
uint8_t iic_read_byte(uint8_t ack);

#endif

