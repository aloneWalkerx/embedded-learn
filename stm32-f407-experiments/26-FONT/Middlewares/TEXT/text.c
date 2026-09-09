#include "string.h"
#include "./TEXT/text.h"
#include "./BSP/LCD/lcd.h"
#include "./MALLOC/malloc.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/NORFLASH/norflash.h"


/**
 * @brief       获取汉字点阵数据
 * @param       code  : 当前汉字编码(GBK码)，指向2字节GBK内码的指针
 * @param       mat   : 当前汉字点阵数据存放地址（输出缓冲区）
 * @param       size  : 字体大小（12/16/24）
 * @note        size大小的字体,其点阵数据大小为: (size/8 + ((size%8)?1:0)) * (size) 字节
 *              例如16号字：16/8=2，每行2字节，共16行 → 32字节
 * @retval      无
 */
static void text_get_hz_mat(unsigned char *code, unsigned char *mat, uint8_t size)
{
    unsigned char qh, ql;          // 分别存储GBK内码的高字节和低字节
    unsigned char i;               // 循环计数器
    unsigned long foffset;         // 该汉字在字库文件中的偏移量（字节）
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size); /* 计算一个字符点阵所占的字节数 */

    qh = *code;                    // 获取GBK高字节（区码）
    ql = *(++code);                // 指针后移，获取GBK低字节（位码）

    /* 判断是否为合法GBK汉字 */
    if (qh < 0x81 || ql < 0x40 || ql == 0xff || qh == 0xff)     /* 非 常用汉字 */
    {
        /* 非法字符：填充全0（显示为空白） */
        for (i = 0; i < csize; i++)
        {
            *mat++ = 0x00;  /* 填充满格，即不显示任何点 */
        }
        return;     /* 结束访问 */
    }

    /* 计算GBK位码的偏移（核心算法） */
    if (ql < 0x7f)                // 当低位 < 0x7F 时
    {
        ql -= 0x40;               // 减去 0x40
    }
    else                          // 当低位 >= 0x7F 时
    {
        ql -= 0x41;               // 减去 0x41（因为0x7F是保留位）
    }

    qh -= 0x81;                   // 高位减去 0x81（GBK起始区号）
    /*
     * GBK编码排列：每区190个汉字
     * 公式：偏移量 = ((区号 * 190) + 位号) × 每个汉字点阵字节数
     * 此处 qh 已是0-based区号，ql 已是0-based位号
     */
    foffset = ((unsigned long)190 * qh + ql) * csize;   /* 得到字库中的字节偏移量 */

    /* 根据字号从不同的NOR Flash地址读取点阵数据 */
    switch (size)
    {
        case 12:
            /* ftinfo.f12addr：12号字库在NOR Flash中的起始地址 */
            norflash_read(mat, foffset + ftinfo.f12addr, csize);
            break;

        case 16:
            /* ftinfo.f16addr：16号字库在NOR Flash中的起始地址 */
            norflash_read(mat, foffset + ftinfo.f16addr, csize);
            break;

        case 24:
            /* ftinfo.f24addr：24号字库在NOR Flash中的起始地址 */
            norflash_read(mat, foffset + ftinfo.f24addr, csize);
            break;

        /* 其他字号不支持，不做处理（实际调用前已做判断） */
    }
}

/**
 * @brief       显示一个指定大小的汉字
 * @param       x,y   : 汉字的起始坐标（像素）
 * @param       font  : 指向汉字GBK码的指针（2字节）
 * @param       size  : 字体大小（12/16/24）
 * @param       mode  : 显示模式
 * @note        0, 正常显示(不需要显示的点,用LCD背景色填充,即g_back_color)
 * @note        1, 叠加显示(仅显示需要显示的点, 不需要显示的点, 不做处理)
 * @param       color : 字体颜色
 * @retval      无
 */
void text_show_font(uint16_t x, uint16_t y, uint8_t *font, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t temp, t, t1;           // temp：当前字节的点阵数据；t：行循环；t1：位循环
    uint16_t y0 = y;               // 保存起始Y坐标，用于行换行判断
    uint8_t *dzk;                  // 指向点阵数据缓冲区的指针
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size);     /* 一个字符点阵所占的字节数 */

    /* 检查字号是否支持（只支持12/16/24） */
    if (size != 12 && size != 16 && size != 24 && size != 32)
    {
        return;     /* 不支持的size，直接退出 */
    }

    dzk = mymalloc(SRAMIN, size);   /* 从内部SRAM内存池申请size字节内存 */

    if (dzk == 0) return;           /* 内存申请失败，直接退出 */

    text_get_hz_mat(font, dzk, size);   /* 从字库中获取该汉字的点阵数据 */

    /*
     * 点阵数据解析与显示
     * 点阵数据按行存储，每行 size/8 字节
     * 每个字节的8个bit对应一行中的8个像素
     */
    for (t = 0; t < csize; t++)    // 遍历每个字节
    {
        temp = dzk[t];              /* 取当前字节的点阵数据 */

        for (t1 = 0; t1 < 8; t1++) // 遍历字节的8个bit
        {
            if (temp & 0x80)        // 检查最高位是否为1
            {
                /* 最高位为1 → 该像素需要显示 → 画前景色 */
                lcd_draw_point(x, y, color);        /* 画需要显示的点 */
            }
            else if (mode == 0)     /* 如果非叠加模式(即正常模式) */
            {
                /* 最高位为0 → 该像素不需要显示 → 用背景色填充 */
                lcd_draw_point(x, y, g_back_color); /* 填充背景色 */
            }
            /* 叠加模式(mode==1)：位为0时不做任何处理，保留原有像素 */

            temp <<= 1;             // 左移一位，处理下一位
            y++;                    // Y坐标下移一行

            /* 检查是否已绘制完一行 */
            if ((y - y0) == size)   // 当Y坐标偏移量等于字体大小时
            {
                y = y0;             // Y坐标回到行首
                x++;                // X坐标右移一列，准备绘制下一列
                break;              // 跳出当前行的位循环
            }
        }
    }

    myfree(SRAMIN, dzk);    /* 释放之前申请的内存 */
}

/**
 * @brief       在指定位置开始显示一个字符串（支持中英文混排、自动换行）
 * @param       x,y   : 起始坐标（像素）
 * @param       width : 显示区域宽度（像素），超出自动换行
 * @param       height: 显示区域高度（像素），超出停止显示
 * @param       str   : 要显示的字符串（GBK编码，以'\0'结尾）
 * @param       size  : 字体大小（12/16/24）
 * @param       mode  : 显示模式（0=正常，1=叠加）
 * @param       color : 字体颜色
 * @retval      无
 */
void text_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *str, uint8_t size, uint8_t mode, uint16_t color)
{
    uint16_t x0 = x;               // 保存起始X坐标，用于换行时复位
    uint16_t y0 = y;               // 保存起始Y坐标，用于越界判断
    uint8_t bHz = 0;               // 标记是否正在处理中文（0=英文，1=中文）
    uint8_t *pstr = (uint8_t *)str; /* 指向字符串首地址（无符号类型便于比较） */

    while (*pstr != 0)             /* 遍历字符串，直到遇到结束符'\0' */
    {
        if (!bHz)                  /* 如果当前不是中文模式（即正在处理英文） */
        {
            if (*pstr > 0x80)      /* 如果当前字节 > 0x80 → 这是中文的第一个字节 */
            {
                bHz = 1;           /* 标记为中文模式，下一个字节是中文的低字节 */
            }
            else    /* 当前是英文字符（ASCII < 0x80） */
            {
                /* 检查是否需要换行：英文字符宽度 = size/2 */
                if (x > (x0 + width - size / 2))    /* 当前行剩余宽度不够显示一个英文 */
                {
                    y += size;      // 换行：Y坐标下移一行
                    x = x0;         // X坐标回到起始位置
                }

                /* 检查是否超出显示区域 */
                if (y > (y0 + height - size)) break; /* 越界返回 */

                if (*pstr == 13)   /* 换行符号（回车符 \r，ASCII=13） */
                {
                    y += size;      // 换行
                    x = x0;         // X回到行首
                    pstr++;         // 跳过这个换行符
                }
                else
                {
                    /* 显示一个英文字符（使用LCD的ASCII字符显示函数） */
                    lcd_show_char(x, y, *pstr, size, mode, color);   /* 有效部分写入 */
                }

                pstr++;             /* 指针后移，处理下一个字符 */
                x += size / 2;      /* 英文字符宽度，为汉字宽度的一半 */
            }
        }
        else    /* 当前是中文模式（正在处理中文的第二字节） */
        {
            bHz = 0;    /* 重置中文标记（因为一个中文已处理完毕） */

            /* 检查是否需要换行：中文宽度 = size */
            if (x > (x0 + width - size))        /* 当前行剩余宽度不够显示一个中文 */
            {
                y += size;          // 换行
                x = x0;             // X回到行首
            }

            /* 检查是否超出显示区域 */
            if (y > (y0 + height - size)) break; /* 越界返回 */

            /* 显示这个汉字：pstr指向当前汉字的GBK内码（2字节） */
            text_show_font(x, y, pstr, size, mode, color); /* 显示这个汉字 */
            pstr += 2;              /* 指针后移2字节，跳过整个汉字的GBK内码 */
            x += size;              /* 下一个字符偏移一个汉字的宽度 */
        }
    }
}

/**
 * @brief       在指定宽度的中间显示字符串（居中显示）
 * @param       x,y   : 区域的起始坐标（左上角）
 * @param       str   : 要显示的字符串
 * @param       size  : 字体大小
 * @param       width : 区域的宽度（像素），字符串将在其中居中
 * @param       color : 字体颜色
 * @retval      无
 * @note        如果字符串长度超过了区域宽度，则自动退回到普通显示模式
 */
void text_show_string_middle(uint16_t x, uint16_t y, char *str, uint8_t size, uint16_t width, uint16_t color)
{
    uint16_t strlenth = 0;         // 字符串占用的总像素宽度

    /* 计算字符串总宽度：每个字符宽度为 size/2（中英文统一按英文字符宽度估算） */
    strlenth = strlen((const char *)str);  // 获取字符个数
    strlenth *= size / 2;                 // 乘以单字符宽度（近似值）

    /* 判断字符串宽度是否超过显示区域 */
    if (strlenth > width) /* 超过了，无法居中显示 → 退化为普通显示模式 */
    {
        /* 使用普通显示模式（mode=1叠加模式） */
        text_show_string(x, y, lcddev.width, lcddev.height, str, size, 1, color);
    }
    else
    {
        /* 计算偏移量，使字符串居中：(区域宽度 - 字符串宽度) / 2 */
        strlenth = (width - strlenth) / 2;
        /* 在偏移后的位置显示字符串 */
        text_show_string(strlenth + x, y, lcddev.width, lcddev.height, str, size, 1, color);
    }
}













