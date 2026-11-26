/**
 ******************************************************************
 * @file        flash.c
 * @author      <aloneWalker>
 * @version     V1.0
 * @date        2026-11-26
 * @brief       STM32F407 内部 Flash 擦写 + CRC32
 ******************************************************************
 * @attention
 *
 * 实验平台:正点原子 M144Z-M4最小系统板STM32F407
 * 本文件遵循正点原子例程的文件格式：宏在 .h，.c 只使用宏
 *
 ******************************************************************
 */

#include "flash.h"
#include "diag_cfg.h"

// 固件长度与 CRC，由 uds_flash.c 在下载过程中维护
//有效固件字节数（不含末尾那 4 字节 CRC）
uint32_t g_fw_len = 0;
//PC 算好、追加在镜像末尾的 CRC32 值
uint32_t g_fw_crc = 0;
/**
 * @brief  解锁 Flash 控制寄存器
 * @return FLASH_OK = 解锁成功；FLASH_ERR = 失败
 * @note   为了保护程序不被乱改，单片机默认是锁死 Flash 写入的。
 *         必须往特定寄存器连续写入两个“暗号”（KEY1, KEY2）才能解锁。
 */
uint8_t flash_unlock(void)
{
    /* HAL_FLASH_Unlock() 内部就是往 FLASH->KEYR 依次写 KEY1、KEY2，
       顺手清掉 SR/CR 里的错误标志；返回 HAL_OK 才算真解锁了 */
    return (HAL_FLASH_Unlock() == HAL_OK) ? FLASH_OK : FLASH_ERR;
}

/**
 * @brief  上锁 Flash 控制寄存器
 * @note   写完程序必须立刻上锁。这就好比金库大门，开完必须马上关上，
 *         防止程序“跑飞了”跑到写 Flash 的代码段，把你的 Bootloader 给误擦除。
 */
void flash_lock(void)
{
    /* 用完立刻上锁，"程序跑飞误写 Flash"的概率才最低 */
    HAL_FLASH_Lock();
}

/**
 * @brief  擦除指定的 Flash 扇区
 * @param  sector: 要擦除的扇区号（0~11）
 * @return FLASH_OK = 成功；FLASH_ERR = 失败
 * @note   注意：Flash 的物理特性是“只能写 1 变成 0，不能写 0 变成 1”。
 *         所以要想写入新数据，必须先“擦除”（把整块全变成 1）。
 *         擦除操作极其耗时（1~2秒），期间 CPU 会卡住等。
 */
uint8_t flash_erase_sector(uint32_t sector)
{
    FLASH_EraseInitTypeDef init;                    /* HAL 要求的擦除参数结构体 */
    uint32_t sector_error = 0;                      /* 出错的扇区号，HAL 用它回填 */

    /* 扇区号越界直接拒绝：F407 只有 0~11，传 12 会让硬件乱来 */
    if (sector >= FLASH_SECTOR_NUM) {
        return FLASH_ERR;
    }

    init.TypeErase = FLASH_TYPEERASE_SECTORS;       /* 按扇区擦，不是整片擦 */
    init.VoltageRange = FLASH_VRANGE_3V3;           /* 3.3V → x32，对应原来 PSIZE=10 */
    init.Sector = sector;                           /* 从哪个扇区开始 */
    init.NbSectors = 1;                             /* 只擦 1 个，不多擦 */

    /* 这个调用是阻塞的：扇区 4 约 0.5~1 秒，128KB 的扇区约 1~2 秒，
       8 个扇区加起来最坏 15 秒 —— 这就是 0x78（响应挂起）必须提前发的原因 */
    if (HAL_FLASHEx_Erase(&init, &sector_error) != HAL_OK) {
        return FLASH_ERR;                           /* 失败：多半是没解锁 */
    }
    return FLASH_OK;
}

/**
 * @brief  往 Flash 里写入数据
 * @param  addr: 要写入的起始地址
 * @param  buf: 要写入的数据
 * @param  len: 数据长度（必须是4的倍数）
 * @return FLASH_OK = 成功；FLASH_ERR = 失败
 * @note   STM32 的 Flash 每次最少必须写 4 个字节（32位整字），不能写 1 个字节。
 *         而且写入之前，这片区域必须已经被擦除过。
 */
uint8_t flash_program(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint32_t i, w;

    /* 门 1：地址不得低于 APP 区，否则会把 Bootloader 自己写坏 */
    if (addr < APP_ADDR) {
        return FLASH_ERR;
    }
    /* 门 2：len & 0x03 非 0，说明不是 4 的倍数；硬件只接受整字 */
    if (len & 0x03u) {
        return FLASH_ERR;
    }
    /* 门 3：先解锁，否则 HAL_FLASH_Program 会直接返回错误 */
    if (flash_unlock() != FLASH_OK) {
        return FLASH_ERR;
    }

    for (i = 0; i < len; i += 4) {                  /* 每次前进 4 字节 = 1 个字 */
        /* 把 4 个字节拼成 32 位字：小端，buf[i] 是最低字节。
           不能直接写成 (uint32_t *)buf —— buf 不保证 4 字节对齐 */
        w = (uint32_t)buf[i] |
        ((uint32_t)buf[i + 1] << 8) |
        ((uint32_t)buf[i + 2] << 16) |
        ((uint32_t)buf[i + 3] << 24);

        /* FLASH_TYPEPROGRAM_WORD = 按 32 位写；HAL 会等 BSY 清零再返回。
           写到"没擦过"的地址不会报错，但数据是错的 —— 所以必须先擦 */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, (uint64_t)w) != HAL_OK) {
            flash_lock();                           /* 失败也要上锁，别把锁留着 */
            return FLASH_ERR;
        }
    }

    flash_lock();                                   /* 成功路径：写完立刻上锁 */
    return FLASH_OK;
}

/**
 * @brief  计算指定内存区域的 CRC32 校验码
 * @param  addr: 起始地址
 * @param  len: 数据长度
 * @return 计算出的 32 位 CRC 值
 * @note   CRC 的作用就像“数据指纹”。只要数据改动了一个字节，算出来的 CRC 就会面目全非。
 *         用于刷机完成后，验证数据是不是百分百传输正确。
 */
static uint32_t crc_calc(uint32_t addr, uint32_t len)
{
    const uint8_t *p = (const uint8_t *)addr;       /* 把地址当字节数组用 */
    uint32_t crc = CRC_INIT_VAL;                    /* 初值全 1，不是 0 */
    uint32_t i, k, b, w;

    for (i = 0; i < len; i += 4) {                  /* 一次吃 4 字节 = 1 个字 */
        w = 0;
        for (k = 0; k < 4; k++) {
            uint32_t idx = i + k;
            /* 尾巴不够 4 字节时补 0（注意：补 0，不是补 0xFF）。
               小端拼字：第 1 个字节放最低 8 位 */
            w |= (uint32_t) ((idx < len) ? p[idx] : 0u) << (8u * k);
        }
        crc ^= w;                                   /* 整字先异或进 CRC */
        for (b = 0; b < 32u; b++) {                 /* 再逐位滚 32 次 */
            /* 最高位是 1 就左移后异或多项式，是 0 就只左移。
               不反射、不反转输入输出，这就是"MSB first"的写法 */
            crc = (crc & 0x80000000u) ? ((crc << 1) ^ CRC_POLY) : (crc << 1);
        }
    }
    return crc;                                     /* 没有最后的输出异或 */
}

/**
 * @brief  擦除整个 APP 区（8个扇区）
 * @return FLASH_OK = 成功；FLASH_ERR = 失败
 * @note   刷机前的第一步。必须把旧程序全部抹掉，否则新程序写不进去。
 */
uint8_t flash_erase_app(void)
{
    uint32_t s;
    uint8_t ret = FLASH_OK;

    /* 整段擦除只解锁一次，减少 KEYR 操作；解不开就直接退出 */
    if (flash_unlock() != FLASH_OK) {
        return FLASH_ERR;
    }

    /* 逐个扇区擦：不能假设扇区等大（4 号 64KB，5~11 号 128KB），
       所以必须一个一个调，"按长度算"那种写法在这里是错的 */
    for (s = APP_SECTOR_FIRST; s <= APP_SECTOR_LAST; s++) {
        if (flash_erase_sector(s) != FLASH_OK) {
            ret = FLASH_ERR;                        /* 记住失败，但后面还得上锁 */
            break;                                  /* 已擦掉的扇区回不去，只能重刷 */
        }
    }

    flash_lock();                                   /* 无论成败都上锁 */
    g_fw_len = 0;                                   /* 此时 Flash 里已没有有效固件 */
    return ret;
}

/**
 * @brief  校验 APP 区的固件是否完整正确
 * @return 0 = 通过；1 = 失败（数据丢失或被篡改）
 * @note   刷机完成后，把写进去的数据重新算一遍 CRC，跟诊断仪发来的 CRC 比对。
 */
uint8_t flash_check_app(void)
{
    uint32_t c;

    /* 长度为 0（还没下载过）或超过分区上限（参数明显不对）都算不通过 */
    if (g_fw_len == 0 || g_fw_len > APP_SIZE_MAX) {
        return 1;
    }
    c = crc_calc(APP_ADDR, g_fw_len);               /* 现场重算：约几十万次循环 */
    return (c == g_fw_crc) ? 0 : 1;                 /* 相等 = 通过 = 返回 0 */
}

/**
 * @brief  读取芯片的全球唯一 ID（96位，12字节）
 * @param  out12: 输出缓冲区，必须至少 12 字节
 * @note   每一片 STM32 芯片出厂时，内部都固化了一串绝对唯一的序列号。
 *         可以用来做防克隆、绑定硬件，或者混入安全访问的种子算法中。
 */
void flash_read_uid(uint8_t *out12)
{
    uint32_t id[3];                                 /* 3 个字 = 96 位 */
    uint8_t i;

    id[0] = HAL_GetUIDw0();                         /* 读 0x1FFF7A10 */
    id[1] = HAL_GetUIDw1();                         /* 读 0x1FFF7A14 */
    id[2] = HAL_GetUIDw2();                         /* 读 0x1FFF7A18 */

    for (i = 0; i < 3; i++) {
        /* 每个字按"高字节在前"拆：这样打出来和芯片手册的 UID 顺序一致 */
        out12[i * 4 + 0] = (uint8_t) (id[i] >> 24);
        out12[i * 4 + 1] = (uint8_t) (id[i] >> 16);
        out12[i * 4 + 2] = (uint8_t) (id[i] >> 8);
        out12[i * 4 + 3] = (uint8_t) (id[i]);
    }
}
