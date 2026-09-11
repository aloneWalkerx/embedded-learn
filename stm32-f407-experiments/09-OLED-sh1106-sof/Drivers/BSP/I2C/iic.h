#ifndef __IIC_H
#define __IIC_H

#include "./SYSTEM/delay/delay.h"   // 引入延时函数，用来产生 I2C 时序的微小等待

// ==================== I2C 引脚定义 ====================
// 下面定义了 OLED 用的 SCL（时钟）和 SDA（数据）连接到哪个 GPIO 口
// 这样改一个地方，整个程序都知道用哪个引脚，方便移植

#define OLED_SCL_GPIO_PORT      GPIOB   // SCL 时钟线挂在 GPIOB 口
#define OLED_SCL_GPIO_PIN       GPIO_PIN_3  // SCL 使用 PB3 引脚

#define OLED_SDA_GPIO_PORT      GPIOB   // SDA 数据线也在 GPIOB 口
#define OLED_SDA_GPIO_PIN       GPIO_PIN_5  // SDA 使用 PB5 引脚

// ==================== GPIO 操作宏（直接寄存器操作） ====================
// 这些宏用来快速拉高或拉低 SDA/SCL 引脚，不用每次都写 HAL 库函数，简洁高效

// 拉高 SDA 引脚（输出高电平，1）
#define OLED_SDA_SET()   HAL_GPIO_WritePin(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN, GPIO_PIN_SET)

// 拉低 SDA 引脚（输出低电平，0）
#define OLED_SDA_RESET()   HAL_GPIO_WritePin(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN, GPIO_PIN_RESET)

// 拉高 SCL 引脚（时钟高电平）
#define OLED_SCL_SET()   HAL_GPIO_WritePin(OLED_SCL_GPIO_PORT, OLED_SCL_GPIO_PIN, GPIO_PIN_SET)

// 拉低 SCL 引脚（时钟低电平）
#define OLED_SCL_RESET()   HAL_GPIO_WritePin(OLED_SCL_GPIO_PORT, OLED_SCL_GPIO_PIN, GPIO_PIN_RESET)

// 设置 SDA 为输入模式（用于读取从机发送过来的应答信号）
#define OLED_SDA_IN()                               do { \
                                                            GPIO_InitTypeDef gpio_init = {0}; \
                                                            gpio_init.Pin = OLED_SDA_GPIO_PIN; \
                                                            gpio_init.Mode = GPIO_MODE_INPUT; \
                                                            gpio_init.Pull = GPIO_PULLUP; \
                                                            gpio_init.Speed = GPIO_SPEED_FREQ_HIGH; \
                                                            HAL_GPIO_Init(OLED_SDA_GPIO_PORT, &gpio_init); \
                                                        } while(0)

// 设置 SDA 为输出模式（主机向外发送数据时用）
#define OLED_SDA_OUT()                              do { \
                                                            GPIO_InitTypeDef gpio_init = {0}; \
                                                            gpio_init.Pin = OLED_SDA_GPIO_PIN; \
                                                            gpio_init.Mode = GPIO_MODE_OUTPUT_PP; \
                                                            gpio_init.Pull = GPIO_PULLUP; \
                                                            gpio_init.Speed = GPIO_SPEED_FREQ_HIGH; \
                                                            HAL_GPIO_Init(OLED_SDA_GPIO_PORT, &gpio_init); \
                                                        } while(0)

// ==================== I2C 地址 ====================
#define IIC_SLAVE_ADDR  0x78   // OLED 的 I2C 从机地址（固定 0x78，有的屏是 0x7A，注意区分）

// 函数声明：下面这些函数在 iic.c 里具体实现
void IIC_Start(void);          // 产生 I2C 起始信号
void IIC_Stop(void);           // 产生 I2C 停止信号
void IIC_Wait_Ack(void);       // 等待从机（OLED）发送应答信号
void Write_IIC_Byte(uint8_t IIC_Byte);          // 发送一个字节（8 位）
void Write_IIC_Command(uint8_t IIC_Command);    // 发送命令字节（告诉 OLED 要做什么）
void Write_IIC_Data(uint8_t IIC_Data);          // 发送数据字节（要显示的内容）

#endif


