#include "string.h"
#include "./BSP/LCD/lcd.h"
#include "./TEXT/fonts.h"
#include "./MALLOC/malloc.h"
#include "./FATFS/source/ff.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/NORFLASH/norflash.h"


/*
 * ============================================================================
 * 宏定义：字库存储区域规划
 * ============================================================================
 *
 * NOR Flash 地址空间布局（以W25Q128为例，16MB容量）：
 *
 * 地址 0x000000  ┌─────────────────────────────────────────────┐
 *                │  文件系统区域 (FATFS)                       │
 *                │  约 12MB                                    │
 * 地址 0xC00000  ├─────────────────────────────────────────────┤  ← FONTINFOADDR
 *                │  ftinfo (41字节)                           │
 * 地址 0xC00029  ├─────────────────────────────────────────────┤
 *                │  UNIGBK.BIN (约170KB)                     │
 * 地址 0xC2Axxx  ├─────────────────────────────────────────────┤
 *                │  GBK12.FON (约574KB)                      │
 * 地址 0xCBBxxx  ├─────────────────────────────────────────────┤
 *                │  GBK16.FON (约766KB)                      │
 * 地址 0xD79xxx  ├─────────────────────────────────────────────┤
 *                │  GBK24.FON (约1.72MB)                     │
 * 地址 0xF1Cxxx  ├─────────────────────────────────────────────┤
 *                │  用户自由使用区域                           │
 * 地址 0xFFFFFF  └─────────────────────────────────────────────┘
 */

/* 字库区域占用的总扇区数大小(3个字库+unigbk表+字库信息=3238700 字节,约占791个25QXX扇区,一个扇区4K字节) */
#define FONTSECSIZE         791


/* 字库存放起始地址
 * 从第12MB地址开始存放字库
 * 前面12MB大小被文件系统占用（FATFS格式化时占用的空间）
 * 12MB后紧跟3个字库+UNIGBK.BIN,总大小3.09M, 791个扇区,被字库占用了,不能动!
 * 15.10M以后, 用户可以自由使用. 建议用最后的100K字节比较好
 */
#define FONTINFOADDR        12 * 1024 * 1024      // = 12,582,912 字节 (0xC00000)

 
/* 用来保存字库基本信息，地址，大小等 */
/* 该结构体在 fonts.h 中声明为 extern，全局唯一 */
_font_info ftinfo;

/* ============================================================================
 * 字库存放在SD卡中的路径列表
 * 索引 0：UNIGBK.BIN  (GBK→Unicode转换表，用于长文件名支持)
 * 索引 1：GBK12.FON   (12×12点阵字库)
 * 索引 2：GBK16.FON   (16×16点阵字库)
 * 索引 3：GBK24.FON   (24×24点阵字库)
 * ============================================================================ */
char *const FONT_GBK_PATH[4] =
{
    "/SYSTEM/FONT/UNIGBK.BIN",      /* UNIGBK.BIN的存放位置 */
    "/SYSTEM/FONT/GBK12.FON",       /* GBK12的存放位置 */
    "/SYSTEM/FONT/GBK16.FON",       /* GBK16的存放位置 */
    "/SYSTEM/FONT/GBK24.FON",       /* GBK24的存放位置 */
};

/* 更新时的提示信息（与FONT_GBK_PATH索引一一对应） */
char *const FONT_UPDATE_REMIND_TBL[4] =
{
    "Updating UNIGBK.BIN",          /* 提示正在更新UNIGBK.bin */
    "Updating GBK12.FON ",          /* 提示正在更新GBK12 */
    "Updating GBK16.FON ",          /* 提示正在更新GBK16 */
    "Updating GBK24.FON ",          /* 提示正在更新GBK24 */
};

/**
 * @brief       显示当前字体更新进度
 * @param       x, y    : 进度显示的起始坐标（像素）
 * @param       size    : 进度文字的大小
 * @param       totsize : 整个字库文件的大小（字节）
 * @param       pos     : 当前已写入的位置（字节）
 * @param       color   : 进度文字的颜色
 * @retval      无
 * @note        该函数在LCD上显示 "XX%" 格式的进度百分比
 *              只有当进度值发生变化时才刷新显示，减少LCD刷新次数
 */
static void fonts_progress_show(uint16_t x, uint16_t y, uint8_t size, uint32_t totsize, uint32_t pos, uint16_t color)
{
    float prog;                    // 进度百分比（浮点数）
    uint8_t t = 0xFF;              // 上次显示的进度值（初始为无效值0xFF）

    prog = (float)pos / totsize;   // 计算进度比例
    prog *= 100;                   // 转为百分比

    if (t != prog)                 // 只有进度发生变化时才更新显示
    {
        lcd_show_string(x + 3 * size / 2, y, 240, 320, size, "%", color);  // 显示 "%" 符号
        t = prog;                  // 更新保存的进度值

        if (t > 100) t = 100;      // 限制最大为100%

        lcd_show_num(x, y, t, 3, size, color);  /* 显示百分比数值（3位，带前导零） */
    }
}

/**
 * @brief       更新某一个字库文件到NOR Flash
 * @param       x, y    : 提示信息的显示地址
 * @param       size    : 提示信息字体大小
 * @param       fpath   : 字库文件在SD卡中的完整路径
 * @param       fx      : 更新的字库类型
 * @arg                    0 = UNIGBK.BIN
 * @arg                    1 = GBK12.FON
 * @arg                    2 = GBK16.FON
 * @arg                    3 = GBK24.FON
 * @param       color   : 字体颜色
 * @retval      0  : 更新成功
 * @retval      1  : 内存分配失败
 * @retval      2  : 打开文件失败
 * @retval      FRESULT 错误码 : 文件读取失败
 * @note        该函数是 fonts_update_font 的内部辅助函数
 * @note        每次读取4096字节写入NOR Flash，循环直到文件读完
 * @note        写入前会自动计算该字库在NOR Flash中的起始地址
 */
static uint8_t fonts_update_fontx(uint16_t x, uint16_t y, uint8_t size, uint8_t *fpath, uint8_t fx, uint16_t color)
{
    uint32_t flashaddr = 0;        // 当前字库在NOR Flash中的起始地址
    FIL *fftemp;                   // 文件对象指针
    uint8_t *tempbuf;              // 数据缓冲区（4KB）
    uint8_t res;                   // 函数返回值
    uint16_t bread;                // 实际读取的字节数
    uint32_t offx = 0;             // 当前文件偏移量
    uint8_t rval = 0;              // 返回值（0=成功）

    /* 从内存池分配文件对象结构体内存 */
    fftemp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));  /* 分配内存 */
    if (fftemp == NULL) rval = 1;   // 内存分配失败

    /* 从内存池分配4KB数据缓冲区 */
    tempbuf = mymalloc(SRAMIN, 4096);               /* 分配4096个字节空间 */
    if (tempbuf == NULL) rval = 1;   // 内存分配失败

    /* 打开字库文件（只读模式） */
    res = f_open(fftemp, (const TCHAR *)fpath, FA_READ);
    if (res) rval = 2;   /* 打开文件失败 */

    /* 如果前面的操作全部成功，开始写入NOR Flash */
    if (rval == 0)
    {
        /*
         * 【地址计算】
         * 根据字库类型计算该字库在NOR Flash中的起始地址
         * 存储顺序：ftinfo → UNIGBK → GBK12 → GBK16 → GBK24
         * 每个字库的起始地址 = 前一个字库的起始地址 + 前一个字库的大小
         */
        switch (fx)
        {
            case 0: /* 更新 UNIGBK.BIN */
                /* UNIGBK 紧跟在 ftinfo 后面 */
                ftinfo.ugbkaddr = FONTINFOADDR + sizeof(ftinfo);    /* 信息头之后，紧跟UNIGBK转换码表 */
                ftinfo.ugbksize = fftemp->obj.objsize;              /* 从文件对象中获取文件大小 */
                flashaddr = ftinfo.ugbkaddr;                        /* 记录起始地址 */
                break;

            case 1: /* 更新 GBK12.FON */
                /* GBK12 紧跟在 UNIGBK 后面 */
                ftinfo.f12addr = ftinfo.ugbkaddr + ftinfo.ugbksize; /* UNIGBK之后，紧跟GBK12字库 */
                ftinfo.gbk12size = fftemp->obj.objsize;             /* GBK12字库大小 */
                flashaddr = ftinfo.f12addr;                         /* GBK12的起始地址 */
                break;

            case 2: /* 更新 GBK16.FON */
                /* GBK16 紧跟在 GBK12 后面 */
                ftinfo.f16addr = ftinfo.f12addr + ftinfo.gbk12size; /* GBK12之后，紧跟GBK16字库 */
                ftinfo.gbk16size = fftemp->obj.objsize;             /* GBK16字库大小 */
                flashaddr = ftinfo.f16addr;                         /* GBK16的起始地址 */
                break;

            case 3: /* 更新 GBK24.FON */
                /* GBK24 紧跟在 GBK16 后面 */
                ftinfo.f24addr = ftinfo.f16addr + ftinfo.gbk16size; /* GBK16之后，紧跟GBK24字库 */
                ftinfo.gbk24size = fftemp->obj.objsize;             /* GBK24字库大小 */
                flashaddr = ftinfo.f24addr;                         /* GBK24的起始地址 */
                break;
        }

        /* 循环读取并写入数据 */
        while (res == FR_OK)   /* 死循环执行，直到文件读完或出错 */
        {
            /* 从SD卡读取最多4096字节数据到缓冲区 */
            res = f_read(fftemp, tempbuf, 4096, (UINT *)&bread);    /* 读取数据 */
            if (res != FR_OK) break;     /* 读取错误，退出循环 */

            /* 将缓冲区数据写入NOR Flash */
            /* norflash_write 会自动处理跨页写入（W25Qxx的页大小通常为256字节） */
            norflash_write(tempbuf, offx + flashaddr, bread);       /* 从0开始写入bread个数据 */
            offx += bread;                                           // 更新文件偏移量

            /* 显示进度 */
            fonts_progress_show(x, y, size, fftemp->obj.objsize, offx, color);    /* 进度显示 */

            /* 如果实际读取的字节数少于4096，说明文件已读完 */
            if (bread != 4096) break;    /* 读完了 */
        }

        f_close(fftemp);      // 关闭文件
    }

    /* 释放之前分配的内存 */
    myfree(SRAMIN, fftemp);     /* 释放内存 */
    myfree(SRAMIN, tempbuf);    /* 释放内存 */
    
    return res;               // 返回最终结果（0表示成功）
}

/**
 * @brief       更新全部字库文件（从SD卡更新到NOR Flash）
 * @param       x, y    : 提示信息的显示地址
 * @param       size    : 提示信息字体大小
 * @param       src     : 字库来源磁盘路径
 * @arg                "0:" = SD卡
 * @arg                "1:" = NOR Flash（一般不使用）
 * @param       color   : 字体颜色
 * @retval      0  : 全部更新成功
 * @retval      5  : 内存申请失败
 * @retval      1+i : 第i个字库更新失败（i=0~3）
 * @retval      其他 : 文件打开失败
 * @note        该函数会先检查4个字库文件是否都存在
 * @note        然后擦除整个字库区域（优化写入速度，只擦除非空扇区）
 * @note        最后依次更新4个字库文件
 * @note        全部更新完成后，将ftinfo结构体写入NOR Flash起始位置
 * @warning     该函数执行时间较长（可能30秒以上），执行期间会显示进度
 */
uint8_t fonts_update_font(uint16_t x, uint16_t y, uint8_t size, uint8_t *src, uint16_t color)
{
    uint8_t *pname;                // 完整文件路径缓冲区
    uint32_t *buf;                 // 4KB缓冲区（用于读取校验）
    uint8_t res = 0;               // 临时返回值
    uint16_t i, j;                 // 循环变量
    FIL *fftemp;                   // 文件对象指针
    uint8_t rval = 0;              // 最终返回值

    res = 0xFF;                    // 初始化为非0值
    ftinfo.fontok = 0xFF;          // 先标记字库为无效（防止中途断电导致错误）

    /* 从内存池申请内存 */
    pname = mymalloc(SRAMIN, 100);                          /* 申请100字节内存（存放文件路径） */
    buf = mymalloc(SRAMIN, 4096);                           /* 申请4K字节内存（用于擦除校验） */
    fftemp = (FIL *)mymalloc(SRAMIN, sizeof(FIL));          /* 分配文件对象内存 */

    /* 检查内存是否全部申请成功 */
    if (buf == NULL || pname == NULL || fftemp == NULL)
    {
        /* 释放已申请的内存 */
        myfree(SRAMIN, fftemp);
        myfree(SRAMIN, pname);
        myfree(SRAMIN, buf);
        return 5;           /* 内存申请失败 */
    }

    /*
     * 【第一步】检查4个字库文件是否都存在于SD卡中
     * 任何一个文件缺失，都放弃更新
     */
    for (i = 0; i < 4; i++) /* 先查找文件UNIGBK,GBK12,GBK16,GBK24 是否正常 */
    {
        strcpy((char *)pname, (char *)src);                 /* copy src内容到pname（例如 "0:"） */
        strcat((char *)pname, (char *)FONT_GBK_PATH[i]);    /* 追加具体文件路径（如 "/SYSTEM/FONT/GBK16.FON"） */
        res = f_open(fftemp, (const TCHAR *)pname, FA_READ);/* 尝试打开文件 */
        if (res)
        {
            rval |= 1 << 7; /* 标记打开文件失败（最高位置1） */
            break;          /* 出错了,直接退出 */
        }
        f_close(fftemp);    // 关闭文件（这里只做存在性检查）
    }

    myfree(SRAMIN, fftemp); /* 释放文件对象内存 */

    /* 【第二步】如果所有文件都存在，开始更新字库到NOR Flash */
    if (rval == 0)          /* 字库文件都存在 */
    {
        /* 显示擦除提示 */
        lcd_show_string(x, y, 240, 320, size, "Erasing sectors... ", color);            /* 提示正在擦除扇区 */

        /*
         * 【优化】只擦除非空的扇区，提高更新速度
         * 读出一个扇区（4KB），检查是否全为0xFF（已擦除状态）
         * 如果不全为0xFF，则执行擦除操作
         */
        for (i = 0; i < FONTSECSIZE; i++)           /* 遍历字库区域的每个扇区 */
        {
            /* 显示擦除进度 */
            fonts_progress_show(x + 20 * size / 2, y, size, FONTSECSIZE, i, color);     /* 进度显示 */

            /* 读出整个扇区的内容 */
            norflash_read((uint8_t *)buf, ((FONTINFOADDR / 4096) + i) * 4096, 4096);    /* 读出整个扇区的内容 */

            /* 检查该扇区是否全为0xFF（已擦除状态） */
            for (j = 0; j < 1024; j++)              /* 检查1024个32位数据（共4096字节） */
            {
                if (buf[j] != 0xFFFFFFFF) break;    /* 需要擦除 */
            }

            /* 如果扇区不是全0xFF，则需要擦除 */
            if (j != 1024)
            {
                norflash_erase_sector((FONTINFOADDR / 4096) + i); /* 需要擦除的扇区 */
            }
        }

        /*
         * 【第三步】依次更新4个字库文件
         * 顺序：UNIGBK → GBK12 → GBK16 → GBK24
         */
        for (i = 0; i < 4; i++) /* 依次更新UNIGBK,GBK12,GBK16,GBK24 */
        {
            /* 显示当前正在更新的字库名称 */
            lcd_show_string(x, y, 240, 320, size, FONT_UPDATE_REMIND_TBL[i], color);

            /* 构建完整文件路径 */
            strcpy((char *)pname, (char *)src);                     /* copy src内容到pname */
            strcat((char *)pname, (char *)FONT_GBK_PATH[i]);        /* 追加具体文件路径 */

            /* 调用内部函数更新单个字库 */
            res = fonts_update_fontx(x + 20 * size / 2, y, size, pname, i, color);  /* 更新字库 */

            /* 如果更新失败，立即返回错误 */
            if (res)
            {
                myfree(SRAMIN, buf);
                myfree(SRAMIN, pname);
                return 1 + i;    // 返回 1+i 表示第i个字库更新失败
            }
        }

        /* 【第四步】全部更新完成，保存字库信息到NOR Flash */
        ftinfo.fontok = 0xAA;                                           // 标记字库正常
        norflash_write((uint8_t *)&ftinfo, FONTINFOADDR, sizeof(ftinfo));           /* 保存字库信息 */
    }

    /* 释放内存 */
    myfree(SRAMIN, pname);  /* 释放内存 */
    myfree(SRAMIN, buf);
    
    return rval;            /* 无错误返回0 */
}

/**
 * @brief       初始化字体（检查字库是否存在且有效）
 * @param       无
 * @retval      0  : 字库完好，可以使用
 * @retval      1  : 字库丢失或损坏，需要调用 fonts_update_font 更新
 * @note        该函数会连续读取10次ftinfo结构体
 *              连续10次都读到 fontok == 0xAA 才认为字库正常
 *              这种冗余检查可以避免偶然的读取错误
 * @note        通常在 main 函数启动时调用
 * @note        如果返回1，需要调用 fonts_update_font() 更新字库
 */
uint8_t fonts_init(void)
{
    uint8_t t = 0;

    /* 连续读取10次，每次间隔20ms，增强可靠性 */
    while (t < 10)  /* 连续读取10次,都是错误,说明确实是有问题,得更新字库了 */
    {
        t++;
        /* 从NOR Flash中读取ftinfo结构体数据 */
        norflash_read((uint8_t *)&ftinfo, FONTINFOADDR, sizeof(ftinfo));    /* 读出ftinfo结构体数据 */

        /* 检查字库标志位是否为0xAA */
        if (ftinfo.fontok == 0xAA)
        {
            break;       // 字库正常，退出循环
        }
        
        delay_ms(20);    // 等待20ms后重试
    }

    /* 如果10次读取后 fontok 仍不是 0xAA，说明字库有问题 */
    if (ftinfo.fontok != 0xAA)
    {
        return 1;        // 返回1，表示需要更新字库
    }
    
    return 0;            // 返回0，表示字库正常
}
