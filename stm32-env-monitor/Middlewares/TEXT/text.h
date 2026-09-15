#ifndef __TEXT_H
#define __TEXT_H

#include "./TEXT/fonts.h"












/* ASCII 字库声明 */
extern const unsigned char asc2_1206[95][12];
extern const unsigned char asc2_1608[95][16];
extern const unsigned char asc2_2412[95][36];
extern const unsigned char asc2_3216[95][128];
/* ASCII 6×8 字库声明 */
extern const unsigned char F6x8[95][8];
/* ASCII 8×16 字库声明 */
/* ASCII 8×16 字库声明（逐行扫描） */
extern const unsigned char F8X16[95][16];

/* 接口函数声明 */

/**
 * @brief       显示单个字符（汉字或ASCII字符）的点阵
 * @param       x      : 起始X坐标（像素）
 * @param       y      : 起始Y坐标（像素）
 * @param       font   : 点阵数据缓冲区（存放从字库中读取的点阵数据）
 * @param       size   : 字体大小（12/16/24，单位：像素）
 * @param       mode   : 显示模式（0：背景色覆盖，1：背景色透明，即叠加显示）
 * @param       color  : 字符颜色（如 RED, BLUE, GREEN 等宏定义）
 * @retval      无
 */
void lcd_text_show_font(uint16_t x, uint16_t y, uint8_t *font, uint8_t size, uint8_t mode, uint16_t color);

/**
 * @brief       在指定区域显示一个字符串（支持中英文混排）
 * @param       x      : 起始X坐标（像素）
 * @param       y      : 起始Y坐标（像素）
 * @param       width  : 显示区域的宽度（像素），超出自动换行
 * @param       height : 显示区域的高度（像素），超出自动裁剪
 * @param       str    : 要显示的字符串（GBK编码）
 * @param       size   : 字体大小（12/16/24，单位：像素）
 * @param       mode   : 显示模式（0：背景色覆盖，1：背景色透明）
 * @param       color  : 字符颜色（如 RED, BLUE, GREEN 等宏定义）
 * @retval      无
 * @note        该函数是汉字显示的核心接口，自动识别中英文，
 *              英文使用 size/2 宽度，汉字使用 size 宽度
 */
void lcd_text_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *str, uint8_t size, uint8_t mode, uint16_t color);

/**
 * @brief       在指定区域内居中显示一个字符串
 * @param       x      : 区域左上角X坐标（像素）
 * @param       y      : 区域左上角Y坐标（像素）
 * @param       str    : 要显示的字符串（GBK编码）
 * @param       size   : 字体大小（12/16/24，单位：像素）
 * @param       width  : 区域的宽度（像素），字符串将在此宽度内居中
 * @param       color  : 字符颜色（如 RED, BLUE, GREEN 等宏定义）
 * @retval      无
 * @note        常用于标题栏或按钮文字的居中显示
 */
void lcd_text_show_string_middle(uint16_t x, uint16_t y, char *str, uint8_t size, uint16_t width, uint16_t color);

/**
 * @brief       获取汉字点阵数据
 * @param       code  : 当前汉字编码(GBK码)，指向2字节GBK内码的指针
 * @param       mat   : 当前汉字点阵数据存放地址（输出缓冲区）
 * @param       size  : 字体大小（12/16/24）
 * @note        size大小的字体,其点阵数据大小为: (size/8 + ((size%8)?1:0)) * (size) 字节
 *              例如16号字：16/8=2，每行2字节，共16行 → 32字节
 * @retval      无
 */
void lcd_oled_text_get_hz_mat(unsigned char *code, unsigned char *mat, uint8_t size);


#endif

