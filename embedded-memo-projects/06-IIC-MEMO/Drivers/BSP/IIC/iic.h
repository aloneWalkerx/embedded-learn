#ifndef __IIC_H
#define __IIC_H

#include "./SYSTEM/sys/sys.h"

/* ---------- 引脚定义（可修改） ---------- */
#define IIC_SCL_GPIO_PORT           GPIOB
#define IIC_SCL_GPIO_PIN            GPIO_PIN_12
#define IIC_SCL_GPIO_CLK_ENABLE()   do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

#define IIC_SDA_GPIO_PORT           GPIOB
#define IIC_SDA_GPIO_PIN            GPIO_PIN_13
#define IIC_SDA_GPIO_CLK_ENABLE()   do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

/* ---------- IO 操作宏 ---------- */
#define IIC_SCL(x)                  do { (x) ?                                                                  \
                                        HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_SET)   : \
                                        HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_RESET); \
                                    } while (0)

#define IIC_SDA(x)                  do { (x) ?                                                                  \
                                        HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_SET)   : \
                                        HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_RESET); \
                                    } while (0)

/* 读取 SDA 电平：返回 0 或 1 */
#define IIC_SDA_READ                ((HAL_GPIO_ReadPin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN) == GPIO_PIN_RESET) ? 0 : 1)

/* ---------- 函数声明 ---------- */
void iic_init(void);                                /* 初始化 IIC（含总线恢复） */
void iic_start(void);                               /* 产生起始信号 */
void iic_stop(void);                                /* 产生停止信号 */
uint8_t iic_wait_slave_ack(void);                   /* 等待从机应答：0=ACK，1=NACK */
uint8_t iic_send_slave_byte(uint8_t data);          /* 发送一个字节：返回 0=ACK，1=NACK */
uint8_t iic_read_slave_byte(uint8_t ack);           /* 读取一个字节：ack=1 发 ACK，ack=0 发 NACK */
void iic_master_ack(void);                          /* 产生 ACK 信号 */
void iic_master_nack(void);                         /* 产生 NACK 信号 */
void iic_bus_recovery(void);                        /* 总线恢复（SCL 翻转 9 次 + 停止条件） */

#endif
