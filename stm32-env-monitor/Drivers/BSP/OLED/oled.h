#ifndef __OLED_H
#define __OLED_H

#include "./SYSTEM/sys/sys.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// OLED 屏幕宽度（像素）
#define OLED_WIDTH     128
// OLED 屏幕高度（像素）
#define OLED_HEIGHT    64
// OLED 页数（每页 8 像素高，64/8=8）
#define OLED_PAGE_NUM  8

/* ★★★ SH1106 专用：列地址偏移 2 列（0x02）★★★ */
// 列地址低半字节：SH1106 从第 2 列开始
#define XLevelL        0x02   // SH1106 从第 2 列开始
// 列地址高半字节：0x10 表示列地址高 4 位为 0
#define XLevelH        0x10
// 页地址命令：0xB0 表示起始页，实际页号通过低 3 位设置
#define YLevel         0xB0

// 命令标志：0 表示命令
#define OLED_CMD       0
// 数据标志：1 表示数据
#define OLED_DATA      1

// OLED I2C 从机地址（7 位地址为 0x3C，左移一位后为 0x78）
#define OLED_ADDRESS   0x78

// 显存数组声明，大小为 128*64/8 = 1024 字节，每个字节对应 8 个像素
extern uint8_t OLED_Buffer[OLED_WIDTH * OLED_HEIGHT / 8];

/* ===== 基础函数 ===== */
// OLED 初始化函数
void oled_init(void);
// 将显存内容刷新到 OLED 屏幕
void OLED_Display(void);
// 清屏函数，color 为 0 表示黑色，1 表示白色
void OLED_Clear(uint8_t color);
// 设置单个像素点颜色
void OLED_SetPixel(uint16_t x, uint16_t y, uint8_t color);
// 打开 OLED 显示
void OLED_DisplayOn(void);
// 关闭 OLED 显示
void OLED_DisplayOff(void);

/* ===== 字符显示 ===== */
// 显示单个字符，size 为字体大小（如 12、16、24 等），color 为颜色
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size, uint8_t color);
// 显示字符串
void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size, uint8_t color);
// 显示无符号十进制数字，len 为显示位数
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color);
// 显示有符号十进制数字
void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size, uint8_t color);
// 显示十六进制数字
void OLED_ShowHexNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color);
// 显示二进制数字
void OLED_ShowBinNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color);

/* ===== 图形绘制 ===== */
// 画直线
void OLED_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
// 画矩形（空心）
void OLED_DrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
// 画圆（空心）
void OLED_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint8_t color);
// 画三角形（空心）
void OLED_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);
// 显示位图（BMP 图片），bmp 为图像数据指针
void OLED_ShowBMP(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp, uint8_t color);

/* ===== 填充图形 ===== */
// 画实心圆
void OLED_FillCircle(uint16_t x0, uint16_t y0, uint8_t r, uint8_t color);
// 画实心三角形（注意函数名拼写为 Triangel，实际是 Triangle）
void OLED_FillTriangel(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);

/* ===== 中文显示（共用 W25Q128 字库） ===== */
// 显示中文字符串（需要外部字库支持）
void OLED_ShowChinese(uint8_t x, uint8_t y, char *str, uint8_t size, uint8_t color);
// 显示中英混合字符串（mode 可能用于选择反色等模式）
void OLED_ShowString_CN(uint8_t x, uint8_t y, char *str, uint8_t size, uint8_t mode);

// 结束条件编译
#endif

