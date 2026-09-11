#include "./BSP/OLED/oled.h"
#include "./BSP/OLED/oledfont.h"
#include "./BSP/I2C/iic.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

uint8_t OLED_Buffer[OLED_WIDTH * OLED_HEIGHT / 8];

/* ===== 底层 I2C 写命令/数据（硬件 I2C） ===== */
static void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    if (cmd) {
        HAL_I2C_Mem_Write(&hi2c1, OLED_ADDRESS, 0x40, I2C_MEMADD_SIZE_8BIT, &dat, 1, 0x100);
    } else {
        HAL_I2C_Mem_Write(&hi2c1, OLED_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &dat, 1, 0x100);
    }
}

/**
 * @brief  SH1106 初始化（硬件 I2C 版）
 * @note   ★ SH1106 专用初始化序列 ★
 *         1. 电荷泵命令：0xAD 0x8B 0x32（与 SSD1315 不同）
 *         2. 列地址偏移：0x02（与 SSD1315 不同）
 */
void OLED_Init(void)
{
    HAL_Delay(200);

    /* ★★★ SH1106 初始化命令序列 ★★★ */
    OLED_WR_Byte(0xAE, OLED_CMD);   // 关闭显示
    OLED_WR_Byte(0x02, OLED_CMD);   // 列低地址（0x02）
    OLED_WR_Byte(0x10, OLED_CMD);   // 列高地址（0x10）
    OLED_WR_Byte(0x40, OLED_CMD);   // 起始行
    OLED_WR_Byte(0xB0, OLED_CMD);   // 页地址
    OLED_WR_Byte(0x81, OLED_CMD);   // 对比度
    OLED_WR_Byte(0xFF, OLED_CMD);
    OLED_WR_Byte(0xA1, OLED_CMD);   // 段重映射
    OLED_WR_Byte(0xA6, OLED_CMD);   // 正常显示
    OLED_WR_Byte(0xA8, OLED_CMD);   // 多路复用
    OLED_WR_Byte(0x3F, OLED_CMD);
    OLED_WR_Byte(0xAD, OLED_CMD);   // ★ SH1106 电荷泵命令（不同于 SSD1315）★
    OLED_WR_Byte(0x8B, OLED_CMD);   // ★ 电荷泵设置
    OLED_WR_Byte(0x32, OLED_CMD);   // ★
    OLED_WR_Byte(0xC8, OLED_CMD);   // COM 扫描方向
    OLED_WR_Byte(0xD3, OLED_CMD);   // 显示偏移
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0xD5, OLED_CMD);   // 振荡器分频
    OLED_WR_Byte(0x80, OLED_CMD);
    OLED_WR_Byte(0xD9, OLED_CMD);   // 预充电
    OLED_WR_Byte(0x1F, OLED_CMD);
    OLED_WR_Byte(0xDA, OLED_CMD);   // COM 引脚
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0xDB, OLED_CMD);   // VCOM 检测
    OLED_WR_Byte(0x40, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);   // 开启显示

    OLED_Clear(0);
    OLED_Display();
}

/**
 * @brief  刷新显存到屏幕
 * @note   ★ SH1106 列地址从 0x02 开始（XLevelL = 0x02）★
 */
void OLED_Display(void)
{
    uint8_t page, col;
    for (page = 0; page < OLED_PAGE_NUM; page++) {
        OLED_WR_Byte(YLevel + page, OLED_CMD);
        OLED_WR_Byte(XLevelL, OLED_CMD);
        OLED_WR_Byte(XLevelH, OLED_CMD);
        for (col = 0; col < OLED_WIDTH; col++) {
            OLED_WR_Byte(OLED_Buffer[page * OLED_WIDTH + col], OLED_DATA);
        }
    }
}

/* ===== 以下函数与你的参考代码完全一致，直接从你的 oled.c 复制 ===== */
/* ===== 包括：OLED_Clear, OLED_SetPixel, OLED_DisplayOn, OLED_DisplayOff ===== */
/* ===== OLED_ShowChar, OLED_ShowString, OLED_ShowNum, OLED_ShowSignedNum ===== */
/* ===== OLED_ShowHexNum, OLED_ShowBinNum, OLED_DrawLine, OLED_DrawRect ===== */
/* ===== OLED_DrawCircle, OLED_DrawTriangle, OLED_ShowBMP ===== */
/* ===== OLED_FillCircle, OLED_FillTriangel, OLED_ShowChinese ===== */

void OLED_Clear(uint8_t color)
{
    memset(OLED_Buffer, color ? 0xFF : 0x00, sizeof(OLED_Buffer));
    OLED_Display();
}

void OLED_SetPixel(uint16_t x, uint16_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    uint16_t index = (y / OLED_PAGE_NUM) * OLED_WIDTH + x;
    uint8_t mask  = 1 << (y % OLED_PAGE_NUM);
    if (color) OLED_Buffer[index] |= mask;
    else OLED_Buffer[index] &= ~mask;
}

void OLED_DisplayOn(void)
{
    OLED_WR_Byte(0xAD, OLED_CMD);
    OLED_WR_Byte(0x8B, OLED_CMD);
    OLED_WR_Byte(0x32, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);
}

void OLED_DisplayOff(void)
{
    OLED_WR_Byte(0xAE, OLED_CMD);
}

/* ===== 字符显示 ===== */
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t Char_Size, uint8_t mode)
{
    unsigned char c = 0, i = 0, tmp, j = 0;
    c = chr - ' ';
    if (x > OLED_WIDTH - 1) { x = 0; y += 2; }
    if (Char_Size == 16) {
        for (i = 0; i < 16; i++) {
            tmp = mode ? F8X16[c * 16 + i] : ~(F8X16[c * 16 + i]);
            for (j = 0; j < 8; j++) {
                OLED_SetPixel(x + j, y + i, (tmp & (0x80 >> j)) ? 1 : 0);
            }
        }
    } else if (Char_Size == 8) {
        for (i = 0; i < 8; i++) {
            tmp = mode ? F6x8[c][i] : ~(F6x8[c][i]);
            for (j = 0; j < 8; j++) {
                OLED_SetPixel(x + j, y + i, (tmp & (0x80 >> j)) ? 1 : 0);
            }
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, char *chr, uint8_t Char_Size, uint8_t mode)
{
    unsigned char j = 0;
    uint8_t csize;
    if (Char_Size == 16) csize = 8;
    else if (Char_Size == 8) csize = 6;
    else return;
    while (chr[j] != '\0') {
        OLED_ShowChar(x, y, chr[j], Char_Size, mode);
        x += csize;
        if (x > 120) { x = 0; y += Char_Size; }
        j++;
    }
    OLED_Display();
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    char str[12];
    sprintf(str, "%0*d", len, num);
    OLED_ShowString(x, y, str, size, color);
}

void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    char str[12];
    sprintf(str, "%+0*d", len, num);
    OLED_ShowString(x, y, str, size, color);
}

void OLED_ShowHexNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    char str[12];
    sprintf(str, "%0*X", len, num);
    OLED_ShowString(x, y, str, size, color);
}

void OLED_ShowBinNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    char str[33];
    uint8_t i;
    for (i = 0; i < len; i++) {
        str[len - 1 - i] = (num & (1 << i)) ? '1' : '0';
    }
    str[len] = '\0';
    OLED_ShowString(x, y, str, size, color);
}

/* ===== 图形绘制 ===== */
void OLED_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color)
{
    int16_t dx = abs(x2 - x1);
    int16_t dy = abs(y2 - y1);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy, e2;
    while (1) {
        OLED_SetPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}

void OLED_DrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color)
{
    OLED_DrawLine(x1, y1, x2, y1, color);
    OLED_DrawLine(x1, y2, x2, y2, color);
    OLED_DrawLine(x1, y1, x1, y2, color);
    OLED_DrawLine(x2, y1, x2, y2, color);
}

void OLED_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint8_t color)
{
    int16_t x = 0, y = r, d = 3 - 2 * r;
    while (x <= y) {
        OLED_SetPixel(x0 + x, y0 + y, color);
        OLED_SetPixel(x0 + y, y0 + x, color);
        OLED_SetPixel(x0 - x, y0 + y, color);
        OLED_SetPixel(x0 - y, y0 + x, color);
        OLED_SetPixel(x0 + x, y0 - y, color);
        OLED_SetPixel(x0 + y, y0 - x, color);
        OLED_SetPixel(x0 - x, y0 - y, color);
        OLED_SetPixel(x0 - y, y0 - x, color);
        if (d < 0) d = d + 4 * x + 6;
        else { d = d + 4 * (x - y) + 10; y--; }
        x++;
    }
}

void OLED_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color)
{
    OLED_DrawLine(x1, y1, x2, y2, color);
    OLED_DrawLine(x2, y2, x3, y3, color);
    OLED_DrawLine(x3, y3, x1, y1, color);
}

void OLED_ShowBMP(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp, uint8_t color)
{
    uint16_t i, j;
    uint8_t byte_width = (width + 7) / 8;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            uint8_t bit = (bmp[i * byte_width + j / 8] >> (7 - j % 8)) & 0x01;
            OLED_SetPixel(x + j, y + i, bit ? color : !color);
        }
    }
}

/* ===== 填充图形 ===== */
void OLED_FillCircle(uint16_t x0, uint16_t y0, uint8_t r, uint8_t color)
{
    int x, y;
    for (y = -r; y <= r; y++) {
        for (x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) OLED_SetPixel(x0 + x, y0 + y, color);
        }
    }
}

void OLED_FillTriangel(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color)
{
    uint16_t min_x = x1, max_x = x1, min_y = y1, max_y = y1;
    if (x2 < min_x) min_x = x2;
    if (x2 > max_x) max_x = x2;
    if (x3 < min_x) min_x = x3;
    if (x3 > max_x) max_x = x3;
    if (y2 < min_y) min_y = y2;
    if (y2 > max_y) max_y = y2;
    if (y3 < min_y) min_y = y3;
    if (y3 > max_y) max_y = y3;
    int16_t d1, d2, d3;
    for (uint16_t y = min_y; y <= max_y; y++) {
        for (uint16_t x = min_x; x <= max_x; x++) {
            d1 = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);
            d2 = (x3 - x2) * (y - y2) - (y3 - y2) * (x - x2);
            d3 = (x1 - x3) * (y - y3) - (y1 - y3) * (x - x3);
            bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
            if (!(has_neg && has_pos)) OLED_SetPixel(x, y, color);
        }
    }
}

/* ===== 中文显示 ===== */
void OLED_ShowChinese(uint8_t x, uint8_t y, char *str, uint8_t size, uint8_t color)
{
    uint8_t j, k, tmp;
    uint16_t num;
    uint8_t char_index = 0;
    while (*str) {
        if (size == 16) {
            num = sizeof(cfont16) / sizeof(typFNT_GB16);
            if (char_index < num) {
                for (j = 0; j < 32; j++) {
                    tmp = color ? cfont16[char_index].Msk[j] : ~cfont16[char_index].Msk[j];
                    for (k = 0; k < 8; k++) {
                        if (tmp & (0x80 >> k)) OLED_SetPixel(x + (j % 2) * 8 + k, y + j / 2, 1);
                        else OLED_SetPixel(x + (j % 2) * 8 + k, y + j / 2, 0);
                    }
                }
            }
            x += size;
            char_index++;
        } else if (size == 24) {
            num = sizeof(cfont24) / sizeof(typFNT_GB24);
            if (char_index < num) {
                for (j = 0; j < 72; j++) {
                    tmp = color ? cfont24[char_index].Msk[j] : ~cfont24[char_index].Msk[j];
                    for (k = 0; k < 8; k++) {
                        if (tmp & (0x80 >> k)) OLED_SetPixel(x + (j % 3) * 8 + k, y + j / 3, 1);
                        else OLED_SetPixel(x + (j % 3) * 8 + k, y + j / 3, 0);
                    }
                }
            }
            x += size;
            char_index++;
        } else if (size == 32) {
            num = sizeof(cfont32) / sizeof(typFNT_GB32);
            if (char_index < num) {
                for (j = 0; j < 128; j++) {
                    tmp = color ? cfont32[char_index].Msk[j] : ~cfont32[char_index].Msk[j];
                    for (k = 0; k < 8; k++) {
                        if (tmp & (0x80 >> k)) OLED_SetPixel(x + (j % 4) * 8 + k, y + j / 4, 1);
                        else OLED_SetPixel(x + (j % 4) * 8 + k, y + j / 4, 0);
                    }
                }
            }
            x += size;
            char_index++;
        }
        str += 2;
        if (x > OLED_WIDTH - size) { x = 0; y += size; }
    }
}
