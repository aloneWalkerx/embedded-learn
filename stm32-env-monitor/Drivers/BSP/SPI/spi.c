#include "./BSP/SPI/spi.h"

/* ====================================================================
   全局变量定义
   ==================================================================== */

// ★ SPI1 句柄（HAL 库用来管理 SPI 外设的核心结构体）
// 包含了 SPI 的所有配置参数、状态信息和回调函数指针
SPI_HandleTypeDef g_spi1_handle = {0};

/* ====================================================================
   ① SPI1 初始化函数
   ==================================================================== */
/**
 * @brief   初始化SPI
 * @param   无
 * @retval  无
 * @note    配置 SPI1 为主模式，模式 3（CPOL=1, CPHA=1）
 *          ★ 这里配置的是模式 3，不是常见的模式 0！
 *          8 位数据格式，MSB 先传，软件管理 NSS（CS）
 *          预分频 256 → 速度最慢（约 328kHz），适合调试
 */
void spi_init(void)
{
    /* --- 步骤 1：配置 SPI1 核心参数 --- */
    
    // ① 指定 SPI 外设（SPI1）
    g_spi1_handle.Instance = SPI1_SPI;
    
    // ② ★ 工作模式：主机模式
    // 主机模式：产生 SCK 时钟，控制通信
    g_spi1_handle.Init.Mode = SPI_MODE_MASTER;
    
    // ③ ★ 通信方向：双线全双工
    // MOSI 用于发送，MISO 用于接收，同时进行
    g_spi1_handle.Init.Direction = SPI_DIRECTION_2LINES;
    
    // ④ 数据大小：8 位（一字节）
    g_spi1_handle.Init.DataSize = SPI_DATASIZE_8BIT;
    
    // ⑤ ★ 时钟极性（CPOL）：高电平空闲
    // CPOL=1 → SCK 空闲时为高电平
    g_spi1_handle.Init.CLKPolarity = SPI_POLARITY_HIGH;
    
    // ⑥ ★ 时钟相位（CPHA）：第 2 个边沿采样
    // CPHA=1 → 数据在 SCK 下降沿被采样
    // 模式 3 的特征：CPOL=1, CPHA=1
    g_spi1_handle.Init.CLKPhase = SPI_PHASE_2EDGE;
    
    // ⑦ ★ NSS（片选）管理：软件模式
    // 软件管理 NSS 意味着 CS 引脚由用户通过 GPIO 控制
    // 这样可以使用任意 GPIO 作为 CS，更灵活
    g_spi1_handle.Init.NSS = SPI_NSS_SOFT;
    
    // ⑧ ★ 波特率预分频：256 分频
    // SPI 时钟 = PCLK2（84MHz）/ 256 ≈ 328kHz
    // ★ 这是最慢的速度，适合调试，稳定可靠
    // 后续可以通过 spi1_set_speed() 提高速度
    g_spi1_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    
    // ⑨ 数据位顺序：MSB 先传（高位在前）
    g_spi1_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    
    // ⑩ TI 模式：禁用（不使用德州仪器模式）
    g_spi1_handle.Init.TIMode = SPI_TIMODE_DISABLE;
    
    // ? CRC 校验：禁用
    g_spi1_handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    
    // ? CRC 多项式：1（禁用 CRC 时无意义）
    g_spi1_handle.Init.CRCPolynomial = 1;
    
    // ? 调用 HAL 库函数完成 SPI 初始化
    // 此函数会自动调用 HAL_SPI_MspInit 配置 GPIO
    HAL_SPI_Init(&g_spi1_handle);
}

/* ====================================================================
   ② HAL 库 SPI MSP 初始化函数（由 HAL_SPI_Init 自动调用）
   ==================================================================== */
/**
 * @brief   HAL库SPI初始化MSP函数
 * @param   hspi: SPI句柄
 * @retval  无
 * @note    此函数由 HAL_SPI_Init 自动调用，不需要用户手动调用
 *          作用是配置 SPI 的底层硬件资源：
 *          1. 使能 SPI 时钟
 *          2. 使能 GPIO 时钟
 *          3. 配置 SCK、MISO、MOSI 为复用推挽模式
 */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio_init_struct = {0};   // GPIO 配置结构体
    
    // ★ 判断是否是 SPI1
    if (hspi->Instance == SPI1_SPI)
    {
        //步骤 1：使能时钟
        // 使能 SPI1 时钟
        SPI1_SPI_CLK_ENABLE();
        // 使能 GPIOB 时钟（SCK 在 PB3）
        SPI1_SCK_GPIO_CLK_ENABLE();
        // 使能 GPIOB 时钟（MISO 在 PB4）
        SPI1_MISO_GPIO_CLK_ENABLE();
        // 使能 GPIOB 时钟（MOSI 在 PB5）
        SPI1_MOSI_GPIO_CLK_ENABLE();
        
        //步骤 2：配置 SCK（PB3）引脚
        // PB3
        gpio_init_struct.Pin = SPI1_SCK_GPIO_PIN;
        // ★ 复用推挽输出
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        //上拉
        gpio_init_struct.Pull = GPIO_PULLUP;
        // 高速
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        // ★ AF5 = SPI1
        gpio_init_struct.Alternate = SPI1_SCK_GPIO_AF;
        //根据参数初始化相关引脚
        HAL_GPIO_Init(SPI1_SCK_GPIO_PIN_ATYPE, &gpio_init_struct);
        
        //步骤 3：配置 MISO（PB4）引脚
        gpio_init_struct.Pin = SPI1_MISO_GPIO_PIN;
        // ★ 复用推挽输出
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        // 上拉
        gpio_init_struct.Pull = GPIO_PULLUP;
        // 高速
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        // ★ AF5 = SPI1
        gpio_init_struct.Alternate = SPI1_MISO_GPIO_AF;
        //根据参数初始化相关引脚
        HAL_GPIO_Init(SPI1_MISO_GPIO_PIN_ATYPE, &gpio_init_struct);
        
        // 步骤 4：配置 MOSI（PB5）引脚
        gpio_init_struct.Pin = SPI1_MOSI_GPIO_PIN;
        // ★ 复用推挽输出
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        // 上拉
        gpio_init_struct.Pull = GPIO_PULLUP;
        // 高速
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        // ★ AF5 = SPI1
        gpio_init_struct.Alternate = SPI1_MOSI_GPIO_AF;
        //根据参数初始化相关引脚
        HAL_GPIO_Init(SPI1_MOSI_GPIO_PIN_ATYPE, &gpio_init_struct);
        
        /* ★ 注意：CS（片选）引脚不在这里配置！
         * 因为 CS 由具体的外设决定（如 W25Q128 用 PB14）
         * 所以 CS 的配置在 norflash.c 中完成
         */
    }
}

/* ====================================================================
   ③ 设置 SPI1 通信速率
   ==================================================================== */
/**
 * @brief   设置SPI1通信波特率
 * @param   speed: SPI1波特率分频系数
 * @arg     SPI_BAUDRATEPRESCALER_2:   2分频 → 42MHz
 * @arg     SPI_BAUDRATEPRESCALER_4:   4分频 → 21MHz
 * @arg     SPI_BAUDRATEPRESCALER_8:   8分频 → 10.5MHz
 * @arg     SPI_BAUDRATEPRESCALER_16:  16分频 → 5.25MHz
 * @arg     SPI_BAUDRATEPRESCALER_32:  32分频 → 2.625MHz
 * @arg     SPI_BAUDRATEPRESCALER_64:  64分频 → 1.3125MHz
 * @arg     SPI_BAUDRATEPRESCALER_128: 128分频 → 656kHz
 * @arg     SPI_BAUDRATEPRESCALER_256: 256分频 → 328kHz
 * @retval  无
 * @note    SPI1 时钟源 = 84MHz（APB2）
 *          实际 SPI 速率 = 84MHz / 分频系数
 *          例如：分频 8 → 84/8 = 10.5MHz
 *          ★ 注意：需要确保从机支持该速率！
 */
void spi_set_speed(uint32_t speed)
{
    /* --- 步骤 1：禁用 SPI --- */
    // ★ 修改 CR1 寄存器前必须禁用 SPI
    __HAL_SPI_DISABLE(&g_spi1_handle);
    
    /* --- 步骤 2：修改波特率分频系数 --- */
    // ★ 直接操作寄存器：清除原来的分频位，设置新的分频值
    // SPI_CR1_BR_Msk = 波特率控制位掩码（bit3~bit5）
    // 清除分频位
    g_spi1_handle.Instance->CR1 &= ~SPI_CR1_BR_Msk;
    // 设置新的分频值
    g_spi1_handle.Instance->CR1 |= speed;
    
    /* --- 步骤 3：重新使能 SPI --- */
    __HAL_SPI_ENABLE(&g_spi1_handle);
}

/* ====================================================================
   ④ SPI1 读写一个字节（全双工）
   ==================================================================== */
/**
 * @brief   SPI1读写一字节数据
 * @param   txdata: 待写入的一字节数据
 * @retval  读取到的一字节数据
 * @note    ★ SPI 是全双工通信！
 *          发送数据的同时必定会接收数据
 *          1. 向 DR 写入 txdata → 启动发送
 *          2. 硬件自动产生 8 个 SCK 时钟
 *          3. 发送和接收同时完成
 *          4. 从 DR 读取 rxdata 返回
 *          
 *          即使只想接收数据，也必须发送一个数据（通常发送 0xFF）
 *          即使只想发送数据，接收的数据也必须丢弃
 */
uint8_t spi_read_write_byte(uint8_t txdata)
{
    // 存储接收到的数据
    uint8_t rxdata;
    
    /* --- 步骤 1：调用 HAL 库的全双工收发函数 --- */
    // 参数 1：SPI 句柄
    // 参数 2：发送数据缓冲区（txdata）
    // 参数 3：接收数据缓冲区（rxdata）
    // 参数 4：数据长度（1 字节）
    // 参数 5：超时时间（1000ms）
    // ★ 如果返回 HAL_OK，说明收发成功
    if (HAL_SPI_TransmitReceive(&g_spi1_handle, &txdata, &rxdata, 1, 1000) != HAL_OK)
    {
        // 失败返回 0
        return 0;   
    }
    // 返回接收到的数据
    return rxdata;
}

