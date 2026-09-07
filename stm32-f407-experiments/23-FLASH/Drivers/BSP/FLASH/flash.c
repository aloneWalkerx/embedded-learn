#include "./BSP/FLASH/flash.h"

/* ====================================================================
   ① 从指定地址读取一个字（32 位）
   ==================================================================== */
/**
 * @brief   从指定地址读取一字的数据
 * @param   addr: 指定读取数据的地址（必须 4 字节对齐）
 * @retval  读取到的一字数据（32 位）
 * @note    内部 Flash 读取非常简单，直接通过指针访问即可
 *          使用 volatile 关键字防止编译器优化，确保每次从真实地址读取
 */
uint32_t flash_read_word(uint32_t addr)
{
    // ★ 将 addr 强制转换为 volatile uint32_t 指针，然后解引用读取
    // volatile 告诉编译器：这个地址的值可能在外部变化，不要优化
    return (*(volatile uint32_t *)addr);
}

/* ====================================================================
   ② 从指定地址读取指定数量的字
   ==================================================================== */
/**
 * @brief   从指定地址读取指定字的数据
 * @param   addr:   指定读取数据的起始地址（必须 4 字节对齐）
 * @param   buf:    存储读取数据的缓冲区指针（uint32_t 数组）
 * @param   length: 指定读取数据的长度，单位：字（1 字 = 4 字节）
 * @retval  无
 * @note    循环调用 flash_read_word 读取每个字
 *          内部 Flash 读取速度极快，适合大批量读取
 */
void flash_read(uint32_t addr, uint32_t *buf, uint32_t length)
{
    uint32_t index;   // 循环计数器
    
    // ★ 逐字读取
    for (index = 0; index < length; index++)
    {
        buf[index] = flash_read_word(addr);   // 读取当前地址的一个字
        addr += sizeof(uint32_t);                // ★ 地址递增 4 字节（一个字）
    }
    // 循环结束后，buf 中存放了 length 个字的读取数据
}

/* ====================================================================
   ③ 往指定地址不检查地写入指定字的数据
   ==================================================================== */
/**
 * @brief   往指定地址不检查地写入指定字的数据
 * @param   addr:   指定写入数据的地址（必须 4 字节对齐）
 * @param   buf:    存储写入数据的起始地址（uint32_t 数组）
 * @param   length: 指定写入数据的长度，单位：字
 * @retval  无
 * @note    ★★★ 危险函数！★★★
 *          使用前必须确保目标地址已经被擦除（数据全部为 0xFFFFFFFF）
 *          如果目标地址有非 0xFF 的数据，写入会失败或数据错乱
 *          
 *          内部流程：
 *          ① 解锁 Flash（允许写入）
 *          ② 禁用数据缓存（防止数据不一致）
 *          ③ 逐字写入
 *          ④ 启用数据缓存
 *          ⑤ 锁住 Flash（禁止写入）
 */
void flash_write_nocheck(uint32_t addr, uint32_t *buf, uint16_t length)
{
    uint16_t index;   // 循环计数器
    
    /* --- 步骤 1：解锁 Flash --- */
    // ★ 必须解锁后才能写入 Flash
    HAL_FLASH_Unlock();
    
    /* --- 步骤 2：禁用数据缓存 --- */
    // ★ 写入 Flash 期间禁用数据缓存，防止缓存数据与 Flash 不一致
    // 如果不禁止，可能读到旧数据
    __HAL_FLASH_DATA_CACHE_DISABLE();
    
    /* --- 步骤 3：逐字写入 --- */
    for (index = 0; index < length; index++)
    {
        // ★ HAL_FLASH_Program 是 HAL 库提供的 Flash 编程函数
        // 参数 1：编程类型（FLASH_TYPEPROGRAM_WORD = 32 位写入）
        // 参数 2：目标地址（必须 4 字节对齐）
        // 参数 3：要写入的数据（32 位）
        // addr + (index << 2) 等同于 addr + index * 4（每次偏移 4 字节）
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + (index << 2), buf[index]);
    }
    
    /* --- 步骤 4：启用数据缓存 --- */
    // ★ 写入完成后重新启用数据缓存
    __HAL_FLASH_DATA_CACHE_ENABLE();
    
    /* --- 步骤 5：锁住 Flash --- */
    // ★ 锁住 Flash，防止意外写入
    HAL_FLASH_Lock();
}

/* ====================================================================
   ④ 获取 Flash 地址所在的扇区（内部函数）
   ==================================================================== */
/**
 * @brief   获取Flash地址所在的Flash扇区
 * @param   addr: Flash地址（0x08000000 ~ 0x080FFFFF）
 * @retval  Flash扇区号（FLASH_SECTOR_0 ~ FLASH_SECTOR_11）
 * @note    STM32F407 的 Flash 扇区分布：
 *          扇区 0:  0x08000000 ~ 0x08003FFF（16KB）
 *          扇区 1:  0x08004000 ~ 0x08007FFF（16KB）
 *          扇区 2:  0x08008000 ~ 0x0800BFFF（16KB）
 *          扇区 3:  0x0800C000 ~ 0x0800FFFF（16KB）
 *          扇区 4:  0x08010000 ~ 0x0801FFFF（64KB）
 *          扇区 5:  0x08020000 ~ 0x0803FFFF（128KB）
 *          扇区 6:  0x08040000 ~ 0x0805FFFF（128KB）
 *          扇区 7:  0x08060000 ~ 0x0807FFFF（128KB）
 *          扇区 8:  0x08080000 ~ 0x0809FFFF（128KB）
 *          扇区 9:  0x080A0000 ~ 0x080BFFFF（128KB）
 *          扇区 10: 0x080C0000 ~ 0x080DFFFF（128KB）
 *          扇区 11: 0x080E0000 ~ 0x080FFFFF（128KB）
 * 
 *          ★ 本实验推荐使用扇区 10 或 11（远离程序代码区域）
 */
static uint32_t flash_get_flash_sector(uint32_t addr)
{
    // ★ 通过地址范围判断扇区号
    if (addr < 0x08004000)          // 0x08000000 ~ 0x08003FFF
    {
        return FLASH_SECTOR_0;
    }
    else if (addr < 0x08008000)     // 0x08004000 ~ 0x08007FFF
    {
        return FLASH_SECTOR_1;
    }
    else if (addr < 0x0800C000)     // 0x08008000 ~ 0x0800BFFF
    {
        return FLASH_SECTOR_2;
    }
    else if (addr < 0x08010000)     // 0x0800C000 ~ 0x0800FFFF
    {
        return FLASH_SECTOR_3;
    }
    else if (addr < 0x08020000)     // 0x08010000 ~ 0x0801FFFF
    {
        return FLASH_SECTOR_4;
    }
    else if (addr < 0x08040000)     // 0x08020000 ~ 0x0803FFFF
    {
        return FLASH_SECTOR_5;
    }
    else if (addr < 0x08060000)     // 0x08040000 ~ 0x0805FFFF
    {
        return FLASH_SECTOR_6;
    }
    else if (addr < 0x08080000)     // 0x08060000 ~ 0x0807FFFF
    {
        return FLASH_SECTOR_7;
    }
    else if (addr < 0x080A0000)     // 0x08080000 ~ 0x0809FFFF
    {
        return FLASH_SECTOR_8;
    }
    else if (addr < 0x080C0000)     // 0x080A0000 ~ 0x080BFFFF
    {
        return FLASH_SECTOR_9;
    }
    else if (addr < 0x080E0000)     // 0x080C0000 ~ 0x080DFFFF
    {
        return FLASH_SECTOR_10;
    }
    else                            // 0x080E0000 ~ 0x080FFFFF
    {
        return FLASH_SECTOR_11;
    }
}

/* ====================================================================
   ⑤ 往指定地址写入指定字的数据（带自动擦除，推荐使用）
   ==================================================================== */
/**
 * @brief   往指定地址写入指定字的数据
 * @param   addr:   指定写入数据的起始地址（必须 4 字节对齐）
 * @param   buf:    要写入的数据缓冲区指针（uint32_t 数组）
 * @param   length: 指定写入数据的长度，单位：字
 * @retval  无
 * @note    ★★★ 推荐使用此函数 ★★★
 *          内部流程：
 *          ① 检查地址合法性（是否在 Flash 范围内、是否 4 字节对齐）
 *          ② 遍历目标地址范围，检查是否需要擦除
 *          ③ 如果某个地址不是 0xFFFFFFFF，则擦除整个扇区
 *          ④ 调用 stmflash_write_nocheck 写入数据
 *          
 *          ★ 擦除扇区会清除该扇区内所有数据！
 *          确保该扇区没有需要保留的其他数据！
 */
void flash_write(uint32_t addr, uint32_t *buf, uint16_t length)
{
    uint32_t addrx;                        // 当前遍历地址
    uint32_t endaddr;                      // 目标结束地址
    FLASH_EraseInitTypeDef flash_erase_init_struct = {0};  // 擦除配置结构体
    uint32_t sectorerr;                    // 擦除错误记录
    HAL_StatusTypeDef status = HAL_OK;     // 操作状态
    
    /* --- 步骤 1：检查写入地址范围的合法性 --- */
    // ① 地址必须在 Flash 范围内（0x08000000 ~ 0x080FFFFF）
    // ② 结束地址也在 Flash 范围内
    // ③ 地址必须 4 字节对齐（(addr & 3) != 0 表示低 2 位不为 0）
    if ((!IS_FLASH_ADDRESS(addr)) ||
        (!IS_FLASH_ADDRESS(addr + (length * sizeof(uint32_t)) - 1)) ||
        ((addr & 3) != 0))
    {
        return;   // 地址非法，直接返回
    }
    
    /* --- 步骤 2：遍历检查是否需要擦除 --- */
    addrx = addr;
    endaddr = addr + (length << 2);   // ★ length << 2 = length * 4（字节数）
    if (addrx <= FLASH_END)           // 确保地址不超出 Flash 末地址
    {
        while (addrx < endaddr)
        {
            /* 判断当前地址的数据是否已经是 0xFFFFFFFF（已擦除状态） */
            if (flash_read_word(addrx) != 0xFFFFFFFF)
            {
                /* ★ 需要擦除：因为 Flash 只能把 1 写成 0，不能把 0 写成 1
                 * 所以如果目标地址不是 0xFFFFFFFF，必须先擦除
                 */
                
                /* 配置擦除参数 */
                flash_erase_init_struct.TypeErase = FLASH_TYPEERASE_SECTORS;  // 扇区擦除
                flash_erase_init_struct.Banks = FLASH_BANK_1;                // Bank 1（STM32F407 只有一个 Bank）
                flash_erase_init_struct.Sector = flash_get_flash_sector(addrx); // 计算扇区号
                flash_erase_init_struct.NbSectors = 1;                       // 只擦除 1 个扇区
                flash_erase_init_struct.VoltageRange = FLASH_VOLTAGE_RANGE_3; // 电压范围（3.3V）
                
                /* 解锁并执行擦除 */
                HAL_FLASH_Unlock();
                status = HAL_FLASHEx_Erase(&flash_erase_init_struct, &sectorerr);
                HAL_FLASH_Lock();
                
                if (status != HAL_OK)
                {
                    break;   // 擦除失败，跳出循环
                }
            }
            else
            {
                /* 当前地址已经是 0xFFFFFFFF，不需要擦除，继续检查下一个地址 */
                addrx += sizeof(uint32_t);   // 检查下一个字
            }
        }
    }
    
    /* --- 步骤 3：执行写入 --- */
    if (status == HAL_OK)
    {
        while (addr < endaddr)
        {
            /* ★ 逐字写入
             * 每次写入 1 个字，地址递增 4 字节
             * 因为前面已经确保擦除，所以可以直接使用 stmflash_write_nocheck
             */
            flash_write_nocheck(addr, buf, 1);   // 写入 1 个字
            addr += sizeof(uint32_t);               // 地址偏移 4 字节
            buf++;                                   // 缓冲区指针偏移 1 个字
        }
    }
}
