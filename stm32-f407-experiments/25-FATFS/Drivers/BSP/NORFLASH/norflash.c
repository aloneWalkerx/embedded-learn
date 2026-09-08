#include "./BSP/NORFLASH/norflash.h"
#include "./BSP/SPI/spi.h"

/* ====================================================================
   全局变量定义
   ==================================================================== */

// ★ NOR Flash 型号（存储芯片 ID，用于判断芯片类型）
// 在 norflash_init() 中赋值，供其他函数使用（如判断是否需要 4 字节地址模式）
uint16_t g_norflash_type;

/* ====================================================================
   ① NOR Flash 初始化
   ==================================================================== */
/**
 * @brief   初始化NOR Flash
 * @param   无
 * @retval  无
 * @note    1. 配置 CS 引脚为推挽输出，默认拉高
 *          2. 初始化 SPI1
 *          3. 设置 SPI 速率为 21MHz（84MHz / 4）
 *          4. 读取芯片 ID，判断型号
 *          5. W25Q256 需要使能 4 字节地址模式
 */
void norflash_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};   // GPIO 配置结构体
    uint8_t temp;                               // 临时变量（用于读取状态寄存器）
    
    /* --- 步骤 1：使能 CS 引脚的 GPIO 时钟 --- */
    NORFLASH_CS_GPIO_CLK_ENABLE();
    
    /* --- 步骤 2：配置 CS 引脚为推挽输出 --- */
    gpio_init_struct.Pin = NORFLASH_CS_GPIO_PIN;   // PB14
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;   // ★ 推挽输出（SPI 的 CS 不需要开漏）
    gpio_init_struct.Pull = GPIO_PULLUP;           // 上拉
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH; // 高速
    HAL_GPIO_Init(NORFLASH_CS_GPIO_PORT, &gpio_init_struct);
    NORFLASH_CS(1);   // ★ 默认 CS 为高（不选中 Flash）
    
    /* --- 步骤 3：初始化 SPI1 并设置速率 --- */
    spi1_init();                                    // 初始化 SPI1（模式 3，8 位，MSB 先传）
    spi1_set_speed(SPI_BAUDRATEPRESCALER_4);       // ★ 设置速率为 21MHz（84MHz/4）
    // ★ 注意：W25Q128 最高支持 104MHz，这里用 21MHz 非常稳定
    
    /* --- 步骤 4：读取芯片 ID 并保存 --- */
    g_norflash_type = norflash_read_id();           // 读取芯片 ID，保存到全局变量
    
    /* --- 步骤 5：W25Q256 特殊处理（4 字节地址模式） --- */
    if (g_norflash_type == W25Q256)                 // ★ 只有 W25Q256 需要 4 字节地址模式
    {
        temp = norflash_read_sr(3);                 // 读取状态寄存器 3
        if ((temp & 0x01) == 0)                     // ★ bit0 = ADS（地址模式选择），0=3字节，1=4字节
        {
            norflash_write_enable();                // ★ 写使能（修改状态寄存器前必须使能）
            temp |= (1 << 1);                       // ★ bit1 = ADP（上电地址模式），1=4字节地址模式
            norflash_write_sr(3, temp);             // 写入状态寄存器 3
            
            NORFLASH_CS(0);                         // 选中 Flash
            spi1_read_write_byte(NORFLASH_Enable4ByteAddr); // 发送 4 字节地址使能命令
            NORFLASH_CS(1);                         // 释放 Flash
        }
    }
}

/* ====================================================================
   ② 等待 NOR Flash 空闲（内部函数）
   ==================================================================== */
/**
 * @brief   等待NOR Flash空闲
 * @param   无
 * @retval  无
 * @note    循环读取状态寄存器 1，直到 BUSY 位（bit0）变为 0
 *          ★ 擦除或写入操作耗时较长，必须等待完成才能进行下一步操作
 */
static void norflash_wait_busy(void)
{
    while ((norflash_read_sr(1) & 0x01) == 0x01);   /* 等待BUSY位清零（bit0=0） */
}

/* ====================================================================
   ③ 写使能 NOR Flash
   ==================================================================== */
/**
 * @brief   写使能NOR Flash
 * @note    将状态寄存器 1 的 WEL（bit1）置位
 *          ★ 执行写/擦除操作前必须先调用此函数！
 * @param   无
 * @retval  无
 */
void norflash_write_enable(void)
{
    NORFLASH_CS(0);                             /* 使能NOR Flash片选 */
    spi1_read_write_byte(NORFLASH_WriteEnable); /* ★ 发送写使能命令 0x06 */
    NORFLASH_CS(1);                             /* 失能NOR Flash片选 */
}

/* ====================================================================
   ④ 发送地址到 NOR Flash（内部函数）
   ==================================================================== */
/**
 * @brief   向NOR Flash发送地址
 * @note    根据芯片型号的不同，发送3字节或4字节地址
 *          - W25Q256（32MB）需要 4 字节地址（24+8=32 位）
 *          - 其他型号（如 W25Q128，16MB）只需要 3 字节地址（24 位）
 * @param   address: 待发送的地址
 * @retval  无
 */
static void norflash_send_address(uint32_t address)
{
    // ★ W25Q256 需要 4 字节地址模式（先发高 8 位）
    if (g_norflash_type == W25Q256)                     /* 只有W25Q256支持4字节地址模式 */
    {
        spi1_read_write_byte((uint8_t)(address >> 24)); /* 发送 bit31~bit24 地址 */
    }
    // ★ 所有型号都发送低 24 位地址
    spi1_read_write_byte((uint8_t)(address >> 16));     /* 发送 bit23~bit16 地址 */
    spi1_read_write_byte((uint8_t)(address >> 8));      /* 发送 bit15~bit8 地址 */
    spi1_read_write_byte((uint8_t)address);             /* 发送 bit7~bit0 地址 */
}

/* ====================================================================
   ⑤ 读状态寄存器
   ==================================================================== */
/**
 * @brief   读NOR Flash的状态寄存器
 * @note    NOR Flash一共有3个状态寄存器
 *          状态寄存器1（0x05）：
 *          BIT   7   6   5   4   3   2   1   0
 *               SPR  RV  TB BP2 BP1 BP0 WEL BUSY
 *          SPR：状态寄存器保护位，配合WP使用
 *          TB、BP2、BP1、BP0：Flash区域写保护设置
 *          WEL：写使能锁定（1=已使能）
 *          BUSY：忙标记位（1：忙；0：空闲）★ 最常用
 *          
 *          状态寄存器2（0x35）：
 *          BIT   7   6   5   4   3   2   1   0
 *               SUS CMP LB3 LB2 LB1 (R)  QE SRP1
 *          
 *          状态寄存器3（0x15）：
 *          BIT   7       6    5    4   3   2   1   0
 *             HOLD/RST  DRV1 DRV0 (R) (R) WPS ADP ADS
 *          ★ ADP = 上电地址模式（1=4字节地址模式）
 * 
 * @param   regno: 状态寄存器索引号，范围：1~3
 * @retval  状态寄存器的值
 */
uint8_t norflash_read_sr(uint8_t regno)
{
    uint8_t byte;        // 读取到的状态寄存器值
    uint8_t command;     // 要发送的读状态寄存器命令
    
    /* --- 步骤 1：根据寄存器编号选择对应的命令 --- */
    switch (regno)
    {
        case 1:
        {
            command = NORFLASH_ReadStatusReg1;  /* 0x05，读状态寄存器1 */
            break;
        }
        case 2:
        {
            command = NORFLASH_ReadStatusReg2;  /* 0x35，读状态寄存器2 */
            break;
        }
        case 3:
        {
            command = NORFLASH_ReadStatusReg3;  /* 0x15，读状态寄存器3 */
            break;
        }
        default:
        {
            command = NORFLASH_ReadStatusReg1;  /* 默认读状态寄存器1 */
            break;
        }
    }
    
    /* --- 步骤 2：发送读命令并读取状态寄存器值 --- */
    NORFLASH_CS(0);                             /* 选中 Flash */
    spi1_read_write_byte(command);              /* 发送读寄存器命令 */
    byte = spi1_read_write_byte(0xFF);          /* ★ 发送 0xFF 的同时读取一个字节 */
    NORFLASH_CS(1);                             /* 释放 Flash */
    
    return byte;
}

/* ====================================================================
   ⑥ 写状态寄存器
   ==================================================================== */
/**
 * @brief   写NOR Flash的状态寄存器
 * @note    状态寄存器1：
 *          BIT   7   6   5   4   3   2   1   0
 *               SPR  RV  TB BP2 BP1 BP0 WEL BUSY
 *          ★ 写状态寄存器前必须先执行写使能！
 * @param   regno: 状态寄存器索引号，范围：1~3
 * @param   sr   : 待写入状态寄存器的值
 * @retval  无
 */
void norflash_write_sr(uint8_t regno, uint8_t sr)
{
    uint8_t command;     // 要发送的写状态寄存器命令
    
    /* --- 步骤 1：根据寄存器编号选择对应的命令 --- */
    switch (regno)
    {
        case 1:
        {
            command = NORFLASH_WriteStatusReg1; /* 0x01，写状态寄存器1 */
            break;
        }
        case 2:
        {
            command = NORFLASH_WriteStatusReg2; /* 0x31，写状态寄存器2 */
            break;
        }
        case 3:
        {
            command = NORFLASH_WriteStatusReg3; /* 0x11，写状态寄存器3 */
            break;
        }
        default:
        {
            command = NORFLASH_WriteStatusReg1; /* 默认写状态寄存器1 */
            break;
        }
    }
    
    /* --- 步骤 2：发送写命令并写入状态寄存器值 --- */
    NORFLASH_CS(0);                             /* 选中 Flash */
    spi1_read_write_byte(command);              /* 发送写寄存器命令 */
    spi1_read_write_byte(sr);                   /* ★ 写入状态寄存器值 */
    NORFLASH_CS(1);                             /* 释放 Flash */
}

/* ====================================================================
   ⑦ 读 NOR Flash 芯片 ID
   ==================================================================== */
/**
 * @brief   读NOR Flash芯片ID
 * @note    使用 0x90 指令（Manufacturer/Device ID）
 *          发送 3 个哑字节（0），然后读取 2 个字节：
 *          高 8 位 = 制造商 ID（0xEF = Winbond）
 *          低 8 位 = 容量 ID（0x17 = 128Mbit）
 * @param   无
 * @retval  NOR Flash芯片ID（如 0xEF17）
 */
uint16_t norflash_read_id(void)
{
    uint16_t deviceid;   // 16 位芯片 ID
    
    NORFLASH_CS(0);                                     /* 选中 Flash */
    spi1_read_write_byte(NORFLASH_ManufactDeviceID);    /* ★ 发送读ID命令 0x90 */
    spi1_read_write_byte(0);                            /* ★ 发送 3 个哑字节（地址） */
    spi1_read_write_byte(0);
    spi1_read_write_byte(0);
    deviceid = spi1_read_write_byte(0xFF) << 8;         /* ★ 读取高8位字节（制造商 ID） */
    deviceid |= spi1_read_write_byte(0xFF);             /* ★ 读取低8位字节（容量 ID） */
    NORFLASH_CS(1);                                     /* 释放 Flash */
    
    return deviceid;
}

/* ====================================================================
   ⑧ 从 NOR Flash 读取数据
   ==================================================================== */
/**
 * @brief   读NOR Flash
 * @note    从指定地址开始读取指定长度的数据
 *          使用 0x03 指令（标准读），发送 24 位地址后连续读取
 * @param   pbuf   : 读取到数据保存的地址
 * @param   addr   : 指定开始读取的地址
 * @param   datalen: 指定读取数据的字节数
 * @retval  无
 */
void norflash_read(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t i;
    
    NORFLASH_CS(0);                             /* 选中 Flash */
    spi1_read_write_byte(NORFLASH_ReadData);    /* ★ 发送读取命令 0x03 */
    norflash_send_address(addr);                /* ★ 发送 24 位（或 32 位）地址 */
    for (i=0; i<datalen; i++)                   /* 循环读取 datalen 个字节 */
    {
        pbuf[i] = spi1_read_write_byte(0xFF);   /* ★ 发送 0xFF（哑字节）同时读取数据 */
    }
    NORFLASH_CS(1);                             /* 释放 Flash */
}

/* ====================================================================
   ⑨ 页编程（内部函数）
   ==================================================================== */
/**
 * @brief   从NOR Flash指定地址写入指定长度的数据（最多 256 字节）
 * @note    写入数据的长度不能超过指定地址所在页的剩余字节数
 *          ★ W25Q128 的页大小为 256 字节
 *          ★ 写入前必须先写使能！
 * @param   pbuf   : 待写入数据的起始地址
 * @param   addr   : 指定开始写入数据的地址
 * @param   datalen: 指定写入数据的字节数，范围：0~指定地址所在页剩余字节数
 * @retval  无
 */
static void norflash_write_page(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t i;
    
    norflash_write_enable();                    /* ★ 写使能（页编程前必须使能） */
    
    NORFLASH_CS(0);                             /* 选中 Flash */
    spi1_read_write_byte(NORFLASH_PageProgram); /* ★ 发送页编程命令 0x02 */
    norflash_send_address(addr);                /* ★ 发送 24 位（或 32 位）地址 */
    for (i = 0; i < datalen; i++)               /* 写入 datalen 个字节 */
    {
        spi1_read_write_byte(pbuf[i]);          /* ★ 发送数据到 Flash */
    }
    NORFLASH_CS(1);                             /* 释放 Flash */
    
    norflash_wait_busy();                       /* ★ 等待编程完成（BUSY 位清零） */
}

/* ====================================================================
   ⑩ 无校验写入（内部函数）
   ==================================================================== */
/**
 * @brief   无检验从NOR Flash指定地址写入指定长度的数据
 * @note    必须确保所写的地址范围内的数据全部为0xFF，否则写入会失败！
 *          ★ 具有自动换页功能（跨页时自动分多次写入）
 *          ★ 此函数不检查目标地址是否已擦除，调用前需确保地址处已是 0xFF
 * @param   pbuf   : 待写入数据的起始地址
 * @param   addr   : 指定开始写入数据的地址
 * @param   datalen: 指定写入数据的字节数
 * @retval  无
 */
static void norflash_write_nocheck(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t pageremain;   // 当前页剩余的字节数
    
    /* --- 步骤 1：计算当前页剩余空间 --- */
    pageremain = 256 - (addr % 256);            /* ★ 计算当前页剩余的字节数 */
    if (datalen <= pageremain)                  /* 如果数据量不超过页剩余空间 */
    {
        pageremain = datalen;                   /* 全部写入当前页 */
    }
    
    /* --- 步骤 2：循环写入（自动换页） --- */
    while (1)
    {
        norflash_write_page(pbuf, addr, pageremain);    /* ★ 写入当前页 */
        
        if (datalen == pageremain)                      /* 所有数据已写完 */
        {
            break;                                      /* 退出循环 */
        }
        else                                            /* 还有剩余数据 */
        {
            pbuf += pageremain;                         /* 缓冲区指针前移 */
            addr += pageremain;                         /* 地址前移 */
            datalen -= pageremain;                      /* 计算剩余数据长度 */
            if (datalen > 256)                          /* 剩余数据超过一页 */
            {
                pageremain = 256;                       /* 先写一页 */
            }
            else                                        /* 剩余数据不足一页 */
            {
                pageremain = datalen;                   /* 全部写入 */
            }
        }
    }
}

/* ====================================================================
   ? 扇区缓存（用于读-改-写操作）
   ==================================================================== */
// ★ 扇区缓存：大小为 4KB（一个扇区的大小）
// 用于在擦除扇区后恢复非写入区域的数据
static uint8_t g_norflash_buf[4096];

/* ====================================================================
   ? 写入 NOR Flash（带擦除，外部调用）
   ==================================================================== */
/**
 * @brief   写NOR Flash
 * @note    在指定地址开始写入指定长度的数据，该函数带擦除操作
 *          NOR Flash一般是：256个字节为一个Page，4096个字节为一个Sector
 *          擦除的最小单位为Sector（4KB）
 *          
 *          ★ 核心逻辑（读-改-写）：
 *          ① 读取整个扇区到缓存
 *          ② 检查目标地址区域是否全为 0xFF
 *          ③ 如果是 → 直接写入（不需要擦除）
 *          ④ 如果不是 → 擦除整个扇区，修改缓存中的目标区域，再写回整个扇区
 * 
 * @param   pbuf   : 待写入数据的起始地址
 * @param   addr   : 指定开始写入数据的地址
 * @param   datalen: 指定写入数据的字节数
 * @retval  无
 */
void norflash_write(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint32_t secpos;       // 当前扇区索引（0 = 第 1 个 4KB 扇区）
    uint16_t secoff;       // 写入起始地址在扇区内的偏移量（0~4095）
    uint16_t secremain;    // 当前扇区剩余的字节数
    uint16_t i;
    uint8_t *norflash_buf; // 指向扇区缓存的指针
    
    norflash_buf = g_norflash_buf;                                      /* 扇区缓存指针 */
    secpos = addr / 4096;                                               /* 计算扇区索引 */
    secoff = addr % 4096;                                               /* 计算扇区内偏移 */
    secremain = 4096 - secoff;                                          /* 计算当前扇区剩余字节 */
    if (datalen <= secremain)                                           /* 数据不超过当前扇区剩余 */
    {
        secremain = datalen;                                            /* 只处理当前扇区 */
    }
    
    while (1)
    {
        /* --- 步骤 1：读取整个扇区到缓存 --- */
        norflash_read(norflash_buf, secpos * 4096, 4096);               /* 读出当前整个扇区的内容 */
        
        /* --- 步骤 2：检查目标区域是否已全部为 0xFF --- */
        for (i=0; i<secremain; i++)                                     /* 校验数据 */
        {
            if (norflash_buf[secoff + i] != 0xFF)                       /* ★ 如果不是 0xFF，需要擦除 */
            {
                break;
            }
        }
        
        /* --- 步骤 3：根据检查结果决定是否擦除 --- */
        if (i < secremain)                                              /* ★ 需要擦除 */
        {
            norflash_erase_sector(secpos);                              /* ★ 擦除整个扇区（4KB） */
            
            /* ★ 将待写入的数据合并到扇区缓存中 */
            for (i=0; i<secremain; i++)                                 /* 将待写入的数据写入扇区缓存 */
            {
                norflash_buf[i + secoff] = pbuf[i];                     /* 只修改目标区域，其他保持不变 */
            }
            
            norflash_write_nocheck(norflash_buf, secpos * 4096, 4096);  /* ★ 写入整个扇区 */
        }
        else                                                            /* ★ 不需要擦除，直接写入 */
        {
            norflash_write_nocheck(pbuf, addr, secremain);              /* ★ 直接写入目标区域 */
        }
        
        /* --- 步骤 4：判断是否还有剩余数据需要写入 --- */
        if (datalen == secremain)                                       /* 所有数据已写完 */
        {
            break;                                                      /* 退出循环 */
        }
        else                                                            /* 还有剩余数据（跨扇区） */
        {
            secpos++;                                                   /* 扇区索引增1（下一个扇区） */
            secoff = 0;                                                 /* 新扇区偏移量为0 */
            pbuf += secremain;                                          /* 缓冲区指针前移 */
            addr += secremain;                                          /* 地址前移 */
            datalen -= secremain;                                       /* 计算剩余数据长度 */
            if (datalen > 4096)                                         /* 剩余数据超过一个扇区 */
            {
                secremain = 4096;                                       /* 先处理一个扇区 */
            }
            else                                                        /* 剩余数据不足一个扇区 */
            {
                secremain = datalen;                                    /* 全部处理 */
            }
        }
    }
}

/* ====================================================================
   ? 擦除整个 NOR Flash 芯片
   ==================================================================== */
/**
 * @brief   擦除整个NOR Flash芯片
 * @note    发送 0xC7 指令，擦除全部数据
 *          ★ 等待时间超长（约 30~50 秒！）
 *          ★ 使用前请确认是否需要全片擦除
 * @param   无
 * @retval  无
 */
void norflash_erase_chip(void)
{
    norflash_write_enable();                    /* ★ 写使能 */
    norflash_wait_busy();                       /* 等待 Flash 空闲 */
    NORFLASH_CS(0);                             /* 选中 Flash */
    spi1_read_write_byte(NORFLASH_ChipErase);   /* ★ 发送全片擦除命令 0xC7 */
    NORFLASH_CS(1);                             /* 释放 Flash */
    norflash_wait_busy();                       /* ★ 等待擦除完成（耗时很长！） */
}

/* ====================================================================
   ? 擦除 NOR Flash 一个扇区（4KB）
   ==================================================================== */
/**
 * @brief   擦除NOR Flash一个扇区
 * @note    擦除一个扇区约需 30~150 毫秒
 *          擦除后该扇区内所有数据变为 0xFF
 *          ★ 扇区是擦除的最小单位（4KB）
 * @param   saddr: 扇区索引号（0 = 第 1 个扇区）
 *          ★ 注意：这里是扇区索引，不是字节地址！
 *          例如：saddr = 0 → 擦除 0x000000~0x000FFF
 *                saddr = 1 → 擦除 0x001000~0x001FFF
 * @retval  无
 */
void norflash_erase_sector(uint32_t saddr)
{
    saddr *= 4096;                              /* ★ 计算扇区索引对应的字节地址 */
    norflash_write_enable();                    /* ★ 写使能 */
    norflash_wait_busy();                       /* 等待 Flash 空闲 */
    
    NORFLASH_CS(0);                             /* 选中 Flash */
    spi1_read_write_byte(NORFLASH_SectorErase); /* ★ 发送扇区擦除命令 0x20 */
    norflash_send_address(saddr);               /* ★ 发送 24 位（或 32 位）地址 */
    NORFLASH_CS(1);                             /* 释放 Flash */
    norflash_wait_busy();                       /* ★ 等待扇区擦除完成（约 30~150ms） */
}
