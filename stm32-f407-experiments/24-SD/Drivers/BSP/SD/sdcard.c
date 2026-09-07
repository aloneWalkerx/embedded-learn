#include "./BSP/SD/sdcard.h"

/* ====================================================================
   全局变量定义
   ==================================================================== */

// ★ SD 卡句柄（HAL 库用于管理 SDIO 外设）
// 包含了 SDIO 的所有配置参数、状态信息和回调函数指针
SD_HandleTypeDef g_sd_handle = {0};

// ★ SD 卡信息结构体（存储卡容量、类型、块大小等）
// 在 sd_init() 中通过 HAL_SD_GetCardInfo 填充
HAL_SD_CardInfoTypeDef g_sd_card_info = {0};

/* ====================================================================
   ① SD 卡初始化
   ==================================================================== */
/**
 * @brief   初始化SD卡
 * @param   无
 * @retval  初始化结果
 * @arg     0: 初始化成功
 * @arg     1: 初始化失败
 * @arg     2: 总线宽度设置失败
 * @note    初始化流程：
 *          ① 配置 SDIO 参数（时钟边沿、分频、总线宽度等）
 *          ② 调用 HAL_SD_Init 初始化 SD 卡（发送命令识别卡）
 *          ③ 获取 SD 卡信息（容量、类型等）
 *          ④ 配置 4 位总线宽度（提高传输速度）
 */
uint8_t sd_init(void)
{
    /* --- 步骤 1：配置 SDIO 参数 --- */
    
    // ① 指定 SDIO 外设实例
    g_sd_handle.Instance = SDIO;
    
    // ② 时钟边沿：上升沿采样
    // SDIO_CLOCK_EDGE_RISING：数据在 SDIO 时钟的上升沿被采样
    g_sd_handle.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    
    // ③ 时钟旁路：禁用
    // 不使用外部时钟源，使用内部时钟分频产生 SDIO 时钟
    g_sd_handle.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    
    // ④ 时钟节能：禁用
    // 不开启节能模式，保证通信稳定性
    g_sd_handle.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    
    // ⑤ 总线宽度：1 位模式（初始化阶段必须用 1 位）
    // ★ 初始化阶段必须使用 1 位模式，初始化完成后切换到 4 位模式
    g_sd_handle.Init.BusWide = SDIO_BUS_WIDE_1B;
    
    // ⑥ 硬件流控制：禁用
    // 不启用硬件流控制（简化通信）
    g_sd_handle.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    
    // ⑦ 时钟分频：1（不分频）
    // ★ SDIO 时钟 = SDIOCLK / (ClockDiv + 2) = 48MHz / 3 = 16MHz
    // 注：SDIOCLK 通常为 48MHz（来自 PLL）
    g_sd_handle.Init.ClockDiv = 1;
    
    /* --- 步骤 2：初始化 SD 卡 --- */
    // HAL_SD_Init 内部会：
    // ① 调用 HAL_SD_MspInit（配置 GPIO）
    // ② 发送 CMD0（复位）→ CMD8（电压检测）→ ACMD41（初始化）
    // ③ 获取卡类型（SDSC/SDHC/SDXC）
    if (HAL_SD_Init(&g_sd_handle) != HAL_OK)
    {
        return 1;   // 初始化失败
    }
    
    /* --- 步骤 3：获取 SD 卡信息 --- */
    // HAL_SD_GetCardInfo 获取：
    // ① CardType：卡类型（SDSC/SDHC/SDXC）
    // ② LogBlockNbr：逻辑块数量
    // ③ LogBlockSize：逻辑块大小（通常为 512 字节）
    // ④ BlockSize：块大小
    HAL_SD_GetCardInfo(&g_sd_handle, &g_sd_card_info);
    
    /* --- 步骤 4：配置 4 位总线宽度 --- */
    // ★ 切换到 4 位数据模式（D0~D3 同时传输，速度是 1 位的 4 倍）
    // 注意：部分旧 SD 卡不支持 4 位模式，会返回错误
    if (HAL_SD_ConfigWideBusOperation(&g_sd_handle, SDIO_BUS_WIDE_4B) != HAL_OK)
    {
        return 2;   // 4 位模式配置失败（可能卡不支持，但 1 位模式仍可用）
    }
    
    return 0;   // 初始化成功
}

/* ====================================================================
   ② HAL 库 SD 初始化 MSP 函数（由 HAL_SD_Init 自动调用）
   ==================================================================== */
/**
 * @brief   HAL库SD初始化MSP函数
 * @param   hsd: SD句柄
 * @retval  无
 * @note    此函数由 HAL_SD_Init 自动调用，不需要用户手动调用
 *          作用是配置 SDIO 的底层硬件资源：
 *          1. 使能 SDIO 和 GPIO 时钟
 *          2. 配置 D0~D3、SCK、CMD 为复用推挽模式
 */
void HAL_SD_MspInit(SD_HandleTypeDef *hsd)
{
    GPIO_InitTypeDef gpio_init_struct = {0};   // GPIO 配置结构体
    
    // ★ 判断是否是 SDIO 外设
    if (hsd->Instance == SDIO)
    {
        /* --- 步骤 1：使能所有需要的时钟 --- */
        __HAL_RCC_SDIO_CLK_ENABLE();   // 使能 SDIO 外设时钟
        SD_D0_GPIO_CLK_ENABLE();       // 使能 PC8（D0）时钟
        SD_D1_GPIO_CLK_ENABLE();       // 使能 PC9（D1）时钟
        SD_D2_GPIO_CLK_ENABLE();       // 使能 PC10（D2）时钟
        SD_D3_GPIO_CLK_ENABLE();       // 使能 PC11（D3）时钟
        SD_SCK_GPIO_CLK_ENABLE();      // 使能 PC12（SCK）时钟
        SD_CMD_GPIO_CLK_ENABLE();      // 使能 PD2（CMD）时钟
        
        /* --- 步骤 2：配置 D0 引脚（PC8）--- */
        gpio_init_struct.Pin = SD_D0_GPIO_PIN;        // PC8
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;       // ★ 复用推挽输出
        gpio_init_struct.Pull = GPIO_PULLUP;           // 上拉（保证空闲电平稳定）
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH; // 高速（SDIO 时钟可达 24MHz）
        gpio_init_struct.Alternate = SD_D0_GPIO_AF;    // ★ AF12 = SDIO
        HAL_GPIO_Init(SD_D0_GPIO_PORT, &gpio_init_struct);
        
        /* --- 步骤 3：配置 D1 引脚（PC9）--- */
        gpio_init_struct.Pin = SD_D1_GPIO_PIN;        // PC9
        gpio_init_struct.Alternate = SD_D1_GPIO_AF;    // AF12 = SDIO
        HAL_GPIO_Init(SD_D1_GPIO_PORT, &gpio_init_struct);
        
        /* --- 步骤 4：配置 D2 引脚（PC10）--- */
        gpio_init_struct.Pin = SD_D2_GPIO_PIN;        // PC10
        gpio_init_struct.Alternate = SD_D2_GPIO_AF;    // AF12 = SDIO
        HAL_GPIO_Init(SD_D2_GPIO_PORT, &gpio_init_struct);
        
        /* --- 步骤 5：配置 D3 引脚（PC11）--- */
        gpio_init_struct.Pin = SD_D3_GPIO_PIN;        // PC11
        gpio_init_struct.Alternate = SD_D3_GPIO_AF;    // AF12 = SDIO
        HAL_GPIO_Init(SD_D3_GPIO_PORT, &gpio_init_struct);
        
        /* --- 步骤 6：配置 SCK 引脚（PC12）--- */
        gpio_init_struct.Pin = SD_SCK_GPIO_PIN;       // PC12
        gpio_init_struct.Alternate = SD_SCK_GPIO_AF;   // AF12 = SDIO
        HAL_GPIO_Init(SD_SCK_GPIO_PORT, &gpio_init_struct);
        
        /* --- 步骤 7：配置 CMD 引脚（PD2）--- */
        gpio_init_struct.Pin = SD_CMD_GPIO_PIN;       // PD2
        gpio_init_struct.Alternate = SD_CMD_GPIO_AF;   // AF12 = SDIO
        HAL_GPIO_Init(SD_CMD_GPIO_PORT, &gpio_init_struct);
    }
}

/* ====================================================================
   ③ 获取 SD 卡信息
   ==================================================================== */
/**
 * @brief   获取SD卡信息
 * @param   info: SD卡信息结构体指针
 * @retval  获取结果
 * @arg     0: 获取成功
 * @arg     1: 获取失败
 */
uint8_t sd_get_card_info(HAL_SD_CardInfoTypeDef *info)
{
    if (HAL_SD_GetCardInfo(&g_sd_handle, info) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/* ====================================================================
   ④ 读 SD 卡指定数量的块数据
   ==================================================================== */
/**
 * @brief   读SD卡指定数量的块数据
 * @param   buf:    数据保存的起始地址（缓冲区）
 * @param   addr:   块地址（扇区号，0 表示第 0 个块）
 * @param   count:  块数量（1 块 = 512 字节）
 * @retval  读取结果
 * @arg     0: 读取成功
 * @arg     1: 读取失败
 * @note    内部流程：
 *          ① 调用 HAL_SD_ReadBlocks 读取数据
 *          ② 等待 SD 卡进入 TRANSFER 状态（传输完成）
 *          ③ 超时则返回失败
 *          
 *          ★ 地址单位是“块”，不是“字节”
 *          例如：addr=0 表示 MBR（主引导记录）
 */
uint8_t sd_read_disk(uint8_t *buf, uint32_t addr, uint32_t count)
{
    uint32_t timeout = SD_DATATIMEOUT;   // 超时计数器（约 100ms）
    
    /* --- 步骤 1：调用 HAL 库函数读取数据 --- */
    // 参数 1：SD 句柄
    // 参数 2：数据缓冲区
    // 参数 3：起始块地址
    // 参数 4：要读取的块数量
    // 参数 5：超时时间
    if (HAL_SD_ReadBlocks(&g_sd_handle, buf, addr, count, SD_DATATIMEOUT) != HAL_OK)
    {
        return 1;   // 读取失败
    }
    
    /* --- 步骤 2：等待传输完成 --- */
    // ★ 轮询等待 SD 卡状态变为 TRANSFER（传输完成）
    // HAL_SD_CARD_TRANSFER 表示卡处于传输状态（空闲，可以接收新命令）
    while ((HAL_SD_GetCardState(&g_sd_handle) != HAL_SD_CARD_TRANSFER) && (--timeout != 0));
    
    // ★ 如果超时（timeout == 0），说明 SD 卡未能进入传输状态
    if (timeout == 0)
    {
        return 1;   // 超时失败
    }
    
    return 0;   // 读取成功
}

/* ====================================================================
   ⑤ 写 SD 卡指定数量的块数据
   ==================================================================== */
/**
 * @brief   写SD卡指定数量的块数据
 * @param   buf:    要写入的数据缓冲区指针
 * @param   addr:   起始块地址（扇区号）
 * @param   count:  要写入的块数量（1 块 = 512 字节）
 * @retval  写入结果
 * @arg     0: 写入成功
 * @arg     1: 写入失败
 * @note    内部流程：
 *          ① 调用 HAL_SD_WriteBlocks 写入数据
 *          ② 等待 SD 卡进入 TRANSFER 状态
 *          ③ 超时则返回失败
 *          
 *          ★ 写入前需确保 SD 卡未被写保护
 *          ★ 写入操作会覆盖原有数据，请谨慎操作！
 */
uint8_t sd_write_disk(uint8_t *buf, uint32_t addr, uint32_t count)
{
    uint32_t timeout = SD_DATATIMEOUT;   // 超时计数器（约 100ms）
    
    /* --- 步骤 1：调用 HAL 库函数写入数据 --- */
    if (HAL_SD_WriteBlocks(&g_sd_handle, buf, addr, count, SD_DATATIMEOUT) != HAL_OK)
    {
        return 1;   // 写入失败
    }
    
    /* --- 步骤 2：等待传输完成 --- */
    // ★ 轮询等待 SD 卡状态变为 TRANSFER（传输完成）
    while ((HAL_SD_GetCardState(&g_sd_handle) != HAL_SD_CARD_TRANSFER) && (--timeout != 0));
    
    // ★ 如果超时（timeout == 0），说明 SD 卡未能进入传输状态
    if (timeout == 0)
    {
        return 1;   // 超时失败
    }
    
    return 0;   // 写入成功
}

