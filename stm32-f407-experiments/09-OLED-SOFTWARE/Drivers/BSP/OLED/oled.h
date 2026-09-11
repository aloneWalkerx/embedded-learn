#ifndef __OLED_H
#define __OLED_H

#include "./SYSTEM/sys/sys.h"   // 包含系统类型定义（uint8_t 等）
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ==================== 屏幕参数 ====================
#define OLED_WIDTH     128    // 屏幕宽度（列数）
#define OLED_HEIGHT    64     // 屏幕高度（行数）
#define OLED_PAGE_NUM  8      // 页数 = 64/8，因为每页是 8 行像素（OLED 按页组织）

// ==================== 列地址偏移（SH1106 专用） ====================
// SH1106 的列地址从 0x02 开始，不是从 0，所以要加偏移
#define XLevelL        0x02   // 列低地址（低 4 位）
#define XLevelH        0x10   // 列高地址（高 4 位，0x10 表示第 16 列开始）
#define YLevel         0xB0   // 页地址起始（0xB0 + 页号）

// ==================== 命令/数据标志 ====================
#define OLED_CMD       0      // 表示后面发的是命令
#define OLED_DATA      1      // 表示后面发的是数据

// ==================== 显存缓冲区（1024 字节） ====================
// 这个数组在 oled.c 中定义，所有绘图函数都是先操作这个数组，最后统一刷到屏幕
extern uint8_t OLED_Buffer[OLED_WIDTH * OLED_HEIGHT / 8];

// ==================== 公共函数声明 ====================
void OLED_Init(void);               // 初始化 OLED（发初始化命令序列）
void OLED_Display(void);            // 把显存内容刷到屏幕
void OLED_Clear(uint8_t color);     // 清屏（0=黑，1=白）
void OLED_SetPixel(uint16_t x, uint16_t y, uint8_t color); // 画一个点
void OLED_DisplayOn(void);          // 打开显示
void OLED_DisplayOff(void);         // 关闭显示（省电）

// 文本显示
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size, uint8_t color);
void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size, uint8_t color);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color);
void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size, uint8_t color);
void OLED_ShowHexNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color);
void OLED_ShowBinNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color);

// 图形绘制
void OLED_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
void OLED_DrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
void OLED_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint8_t color);
void OLED_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);
void OLED_ShowBMP(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp, uint8_t color);

// 填充图形（用于菜单特效）
void OLED_FillCircle(uint16_t x0, uint16_t y0, uint8_t r, uint8_t color);
void OLED_FillTriangel(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);
void OLED_ShowChinese(uint8_t x, uint8_t y, char *str, uint8_t size, uint8_t color);

#endif

