#ifndef __OLED_H
#define __OLED_H

#include "./SYSTEM/sys/sys.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define OLED_WIDTH     128
#define OLED_HEIGHT    64
#define OLED_PAGE_NUM  8

/* ★★★ SH1106 专用：列地址偏移 2 列（0x02）★★★ */
#define XLevelL        0x02   // SH1106 从第 2 列开始
#define XLevelH        0x10
#define YLevel         0xB0

#define OLED_CMD       0
#define OLED_DATA      1

#define OLED_ADDRESS   0x78

extern uint8_t OLED_Buffer[OLED_WIDTH * OLED_HEIGHT / 8];

/* ===== 基础函数 ===== */
void OLED_Init(void);
void OLED_Display(void);
void OLED_Clear(uint8_t color);
void OLED_SetPixel(uint16_t x, uint16_t y, uint8_t color);
void OLED_DisplayOn(void);
void OLED_DisplayOff(void);

/* ===== 字符显示 ===== */
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size, uint8_t color);
void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size, uint8_t color);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color);
void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size, uint8_t color);
void OLED_ShowHexNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color);
void OLED_ShowBinNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color);

/* ===== 图形绘制 ===== */
void OLED_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
void OLED_DrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
void OLED_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint8_t color);
void OLED_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);
void OLED_ShowBMP(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp, uint8_t color);

/* ===== 填充图形 ===== */
void OLED_FillCircle(uint16_t x0, uint16_t y0, uint8_t r, uint8_t color);
void OLED_FillTriangel(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);

/* ===== 中文显示 ===== */
void OLED_ShowChinese(uint8_t x, uint8_t y, char *str, uint8_t size, uint8_t color);

#endif
