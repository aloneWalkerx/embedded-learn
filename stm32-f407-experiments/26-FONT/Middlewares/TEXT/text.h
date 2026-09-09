#ifndef __TEXT_H
#define __TEXT_H

#include "./TEXT/fonts.h"

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
void text_show_font(uint16_t x, uint16_t y, uint8_t *font, uint8_t size, uint8_t mode, uint16_t color);

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
void text_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *str, uint8_t size, uint8_t mode, uint16_t color);

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
void text_show_string_middle(uint16_t x, uint16_t y, char *str, uint8_t size, uint16_t width, uint16_t color);

#endif

