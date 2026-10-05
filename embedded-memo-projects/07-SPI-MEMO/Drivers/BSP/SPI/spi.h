#ifndef __SPI_H
#define __SPI_H

#include "./SYSTEM/sys/sys.h"

#define SPI_SPIX                SPI1                // 选择使用 SPI1 外设
#define SPI_SPIX_CLK_ENABLE()   do{__HAL_RCC_SPI1_CLK_ENABLE();}while(0) // 使能 SPI1 时钟；do...while(0) 保证宏展开安全

#define SPI_SCK_GPIO_PORT           GPIOB           // SCK 时钟线使用的 GPIO 端口：GPIOB
#define SPI_SCK_GPIO_PIN            GPIO_PIN_3      // SCK 时钟线使用的引脚：PB3
#define SPI_SCK_GPIO_AF             GPIO_AF5_SPI1   // SCK 引脚复用为 SPI1 功能：AF5
#define SPI_SCK_GPIO_CLK_ENABLE()   do{__HAL_RCC_GPIOB_CLK_ENABLE();}while(0) // 使能 GPIOB 时钟（SCK 所在端口）

#define SPI_MISO_GPIO_PORT          GPIOB           // MISO 数据输入线使用的 GPIO 端口：GPIOB
#define SPI_MISO_GPIO_PIN           GPIO_PIN_4      // MISO 数据输入线使用的引脚：PB4
#define SPI_MISO_GPIO_AF            GPIO_AF5_SPI1   // MISO 引脚复用为 SPI1 功能：AF5
#define SPI_MISO_GPIO_CLK_ENABLE()  do{__HAL_RCC_GPIOB_CLK_ENABLE();}while(0); // 使能 GPIOB 时钟（MISO 所在端口）；注意：原宏末尾多了一个分号

#define SPI_MOSI_GPIO_PORT          GPIOB           // MOSI 数据输出线使用的 GPIO 端口：GPIOB
#define SPI_MOSI_GPIO_PIN           GPIO_PIN_5      // MOSI 数据输出线使用的引脚：PB5
#define SPI_MOSI_GPIO_AF            GPIO_AF5_SPI1   // MOSI 引脚复用为 SPI1 功能：AF5
#define SPI_MOSI_GPIO_CLK_ENABLE()  do{__HAL_RCC_GPIOB_CLK_ENABLE();}while(0) // 使能 GPIOB 时钟（MOSI 所在端口）

void spi_init(void);                                // 声明 SPI 初始化函数
void spi_set_speed(uint32_t speed);                 // 声明 SPI 速度设置函数，speed 为速度参数
uint8_t spi_send_read_byte(uint8_t txdata);         // 声明 SPI 收发一个字节函数，txdata 为发送数据，返回接收数据

#endif
