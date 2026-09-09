#ifndef __FONTS_H
#define __FONTS_H

#include "./SYSTEM/sys/sys.h"


/*
 * 【字库信息存储说明】
 *
 * 字库信息保存在 NOR Flash 的起始位置（地址由 FONTINFOADDR 指定）
 * 占用空间：1 + 5 × 8 = 41 字节
 *
 * 存储布局：
 * ┌─────────────┬─────────────┬─────────────┬─────────────┬─────────────┬─────────────┐
 * │  字体OK标志 │  UNIGBK地址  │  UNIGBK大小  │  GBK12地址   │  GBK12大小   │  GBK16地址   │
 * │  1字节      │  4字节      │  4字节      │  4字节      │  4字节      │  4字节      │
 * ├─────────────┼─────────────┼─────────────┼─────────────┼─────────────┼─────────────┤
 * │  GBK16大小  │  GBK24地址   │  GBK24大小   │             │             │             │
 * │  4字节      │  4字节      │  4字节      │             │             │             │
 * └─────────────┴─────────────┴─────────────┴─────────────┴─────────────┴─────────────┘
 */
/* 字体信息保存首地址
 * 占41个字节,第1个字节用于标记字库是否存在.后续每8个字节一组,分别保存起始地址和文件大小
 * 该变量在 fonts.c 中定义，指向 NOR Flash 中存储字库信息的起始地址
 */
extern uint32_t FONTINFOADDR;          // 声明外部变量：字库信息在 NOR Flash 中的存储地址

/* 字库信息结构体定义
 * 用来保存字库基本信息，地址，大小等
 * __PACKED_STRUCT 表示按1字节对齐，确保结构体在内存中的布局与 Flash 存储完全一致
 * 总共：1 + 4×8 = 33 字节（5组地址+大小，每组8字节，加上1字节标志位）
 */
typedef __PACKED_STRUCT
{
    uint8_t fontok;             /* 字库存在标志，0XAA 表示字库正常；其他值表示字库不存在或损坏 */
    uint32_t ugbkaddr;          /* UNIGBK.BIN 字库文件在 NOR Flash 中的起始地址（GBK→Unicode转换表） */
    uint32_t ugbksize;          /* UNIGBK.BIN 文件的大小（字节数） */
    uint32_t f12addr;           /* GBK12.FON 字库文件在 NOR Flash 中的起始地址（12×12点阵） */
    uint32_t gbk12size;         /* GBK12.FON 文件的大小（字节数） */
    uint32_t f16addr;           /* GBK16.FON 字库文件在 NOR Flash 中的起始地址（16×16点阵） */
    uint32_t gbk16size;         /* GBK16.FON 文件的大小（字节数） */
    uint32_t f24addr;           /* GBK24.FON 字库文件在 NOR Flash 中的起始地址（24×24点阵） */
    uint32_t gbk24size;         /* GBK24.FON 文件的大小（字节数） */
} _font_info;

/* 字库信息结构体变量 */
/* 在 fonts.c 中定义，存储从 NOR Flash 读取的字库信息 */
extern _font_info ftinfo;        // 声明外部变量：字库信息结构体


/**
 * @brief       从 SD 卡更新 NOR Flash 中的全部字库
 * @param       x, y    : 更新进度显示的起始坐标（LCD显示用）
 * @param       size    : 进度文字的大小
 * @param       src     : 字库来源路径，如 "0:/SYSTEM/FONT"
 * @param       color   : 进度文字的颜色
 * @retval      0   ：更新成功
 * @retval      1   ：更新失败（SD卡未挂载、字库文件缺失、Flash写入错误等）
 * @note        该函数会从SD卡读取字库文件（UNIGBK.BIN、GBK12.FON、GBK16.FON、GBK24.FON），
 *              并将其写入 NOR Flash 的指定地址区域。
 *              首次使用或字库损坏时需要调用此函数。
 * @note        该函数执行时间较长（可能数十秒），执行期间会显示进度信息
 */
uint8_t fonts_update_font(uint16_t x, uint16_t y, uint8_t size, uint8_t *src, uint16_t color);

/**
 * @brief       初始化字库
 * @retval      0   ：初始化成功（字库已存在且正常）
 * @retval      1   ：初始化失败（字库不存在或损坏，需要调用 fonts_update_font 更新）
 * @note        该函数会从 NOR Flash 中读取字库信息（ftinfo），
 *              检查 fontok 标志是否为 0xAA，并验证各字库的地址和大小是否合理
 * @note        通常在 main 函数启动时调用，确认字库可用后再进行汉字显示
 */
uint8_t fonts_init(void);

#endif

