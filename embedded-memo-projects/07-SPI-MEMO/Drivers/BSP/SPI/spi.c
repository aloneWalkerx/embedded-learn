#include "./BSP/SPI/spi.h"

SPI_HandleTypeDef g_spi_hndle = {0};

void spi_init(void) {
    g_spi_hndle.Instance = SPI_SPIX;                // 选择 SPI 外设实例，这里为 SPI1
    g_spi_hndle.Init.Mode = SPI_MODE_MASTER;        // 设置 SPI 为主机模式
    g_spi_hndle.Init.Direction = SPI_DIRECTION_2LINES; // 设置为双线全双工模式
    g_spi_hndle.Init.DataSize = SPI_DATASIZE_8BIT;  // 数据帧长度设置为 8 位
    g_spi_hndle.Init.FirstBit = SPI_FIRSTBIT_MSB;   // 先发送最高位 MSB
    g_spi_hndle.Init.CLKPolarity = SPI_POLARITY_HIGH; // 时钟空闲状态为高电平，即 CPOL=1
    g_spi_hndle.Init.CLKPhase = SPI_PHASE_2EDGE;    // 在第二个时钟边沿采样，即 CPHA=1
    g_spi_hndle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256; // 波特率预分频设置为 256
    g_spi_hndle.Init.NSS = SPI_NSS_SOFT;            // 片选 NSS 使用软件控制
    g_spi_hndle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE; // 禁用 CRC 校验
    g_spi_hndle.Init.TIMode = SPI_TIMODE_DISABLE;   // 禁用 TI 模式
    g_spi_hndle.Init.CRCPolynomial = 1;             // CRC 多项式设置为 1，禁用 CRC 时无实际作用

    HAL_SPI_Init(&g_spi_hndle);                     // 调用 HAL 库初始化 SPI 外设

}                                                   // spi_init 函数结束

void HAL_SPI_MspInit(SPI_HandleTypeDef *spi) {      // HAL SPI 底层初始化回调函数，重写 HAL 库弱函数
    if (spi->Instance == SPI_SPIX) {                // 判断当前 SPI 实例是否为 SPI1
        SPI_SPIX_CLK_ENABLE();                      // 使能 SPI1 时钟
        SPI_SCK_GPIO_CLK_ENABLE();                  // 使能 SCK 引脚所在 GPIO 端口时钟
        SPI_MISO_GPIO_CLK_ENABLE();                 // 使能 MISO 引脚所在 GPIO 端口时钟
        SPI_MOSI_GPIO_CLK_ENABLE();                 // 使能 MOSI 引脚所在 GPIO 端口时钟

        GPIO_InitTypeDef gpio_init_struct = {0};    // 定义 GPIO 初始化结构体并清零

        gpio_init_struct.Pin = SPI_SCK_GPIO_PIN;    // 选择 SCK 引脚
        gpio_init_struct.Mode = GPIO_MODE_AF_OD;    // 设置为复用开漏模式
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH; // GPIO 速度设置为高速
        gpio_init_struct.Pull = GPIO_PULLUP;        // 启用上拉电阻
        gpio_init_struct.Alternate = SPI_SCK_GPIO_AF; // 复用功能选择 SPI1

        HAL_GPIO_Init(SPI_SCK_GPIO_PORT, &gpio_init_struct); // 初始化 SCK 引脚

        gpio_init_struct.Pin = SPI_MISO_GPIO_PIN;   // 选择 MISO 引脚
        gpio_init_struct.Mode = GPIO_MODE_AF_OD;    // 设置为复用开漏模式
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH; // GPIO 速度设置为高速
        gpio_init_struct.Pull = GPIO_PULLUP;        // 启用上拉电阻
        gpio_init_struct.Alternate = SPI_MISO_GPIO_AF; // 复用功能选择 SPI1

        HAL_GPIO_Init(SPI_MISO_GPIO_PORT, &gpio_init_struct); // 初始化 MISO 引脚

        gpio_init_struct.Pin = SPI_MOSI_GPIO_PIN;   // 选择 MOSI 引脚
        gpio_init_struct.Mode = GPIO_MODE_AF_OD;    // 设置为复用开漏模式
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH; // GPIO 速度设置为高速
        gpio_init_struct.Pull = GPIO_PULLUP;        // 启用上拉电阻
        gpio_init_struct.Alternate = SPI_MOSI_GPIO_AF; // 复用功能选择 SPI1

        HAL_GPIO_Init(SPI_MOSI_GPIO_PORT, &gpio_init_struct); // 初始化 MOSI 引脚

    }                                               // if 判断结束

}                                                   // HAL_SPI_MspInit 函数结束

void spi_speed(uint32_t speed) {                    // 设置 SPI 波特率预分频值，注意头文件中声明为 spi_set_speed
    __HAL_SPI_DISABLE(&g_spi_hndle);                // 修改配置前先关闭 SPI
    g_spi_hndle.Instance->CR1 &= ~SPI_CR1_BR_Msk;   // 清除 CR1 寄存器中的波特率预分频位
    g_spi_hndle.Instance->CR1 |= speed;             // 写入新的波特率预分频值
    __HAL_SPI_ENABLE(&g_spi_hndle);                 // 重新使能 SPI

}                                                   // spi_speed 函数结束

uint8_t spi_send_read_byte(uint8_t txdata) {        // SPI 发送并接收一个字节
    uint8_t rxdata;                                 // 定义接收数据变量
    if (HAL_SPI_TransmitReceive(&g_spi_hndle, &txdata, &rxdata, 1, 1000) != HAL_OK) { // 调用 HAL 库收发 1 字节，超时 1000ms；若失败

        return 0;                                   // 返回 0 表示通信失败

    }                                               // if 判断结束
    return rxdata;                                  // 返回接收到的数据

}                                                   // spi_send_read_byte 函数结束
