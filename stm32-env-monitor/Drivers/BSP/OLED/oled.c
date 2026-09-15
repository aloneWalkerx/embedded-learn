#include "./BSP/OLED/oled.h"
#include "./BSP/IIC/iic.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "./TEXT/text.h"
#include "./BSP/NORFLASH/norflash.h"

// 定义 OLED 显存数组，大小为 128*64/8 = 1024 字节，每个字节对应 8 个像素
uint8_t OLED_Buffer[OLED_WIDTH * OLED_HEIGHT / 8];

/* ===== 底层 I2C 写命令/数据（硬件 I2C） ===== */
/**
 * @brief  向 OLED 写入一个字节（命令或数据）
 * @param  dat : 要写入的字节
 * @param  cmd : 0 表示命令，1 表示数据
 */
static void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    // 如果 cmd 为真（非0），表示写入数据，使用控制字节 0x40
    if (cmd) {
        // 通过硬件 I2C 向 OLED 从机地址写入一个字节，内存地址为 0x40（数据）
        HAL_I2C_Mem_Write(&i2c1def, OLED_ADDRESS, 0x40, I2C_MEMADD_SIZE_8BIT, &dat, 1, 0x100);
    } else {
        // 否则写入命令，控制字节为 0x00
        HAL_I2C_Mem_Write(&i2c1def, OLED_ADDRESS, 0x00, I2C_MEMADD_SIZE_8BIT, &dat, 1, 0x100);
    }
}

/**
 * @brief  SH1106 初始化（硬件 I2C 版）
 * @note   ★ SH1106 专用初始化序列 ★
 *         1. 电荷泵命令：0xAD 0x8B 0x32（与 SSD1315 不同）
 *         2. 列地址偏移：0x02（与 SSD1315 不同）
 */
void oled_init(void)
{
    // 延时 200ms，等待 OLED 上电稳定
    HAL_Delay(200);

    /* ★★★ SH1106 初始化命令序列 ★★★ */
    OLED_WR_Byte(0xAE, OLED_CMD);   // 关闭显示
    OLED_WR_Byte(0x02, OLED_CMD);   // 设置列低地址为 0x02（SH1106 偏移）
    OLED_WR_Byte(0x10, OLED_CMD);   // 设置列高地址为 0x10
    OLED_WR_Byte(0x40, OLED_CMD);   // 设置显示起始行（0x40 + 行号）
    OLED_WR_Byte(0xB0, OLED_CMD);   // 设置页地址（0xB0 + 页号）
    OLED_WR_Byte(0x81, OLED_CMD);   // 设置对比度命令
    OLED_WR_Byte(0xFF, OLED_CMD);   // 对比度值 0xFF（最大）
    OLED_WR_Byte(0xA1, OLED_CMD);   // 段重映射（左右方向）
    OLED_WR_Byte(0xA6, OLED_CMD);   // 正常显示（非反色）
    OLED_WR_Byte(0xA8, OLED_CMD);   // 设置多路复用率
    OLED_WR_Byte(0x3F, OLED_CMD);   // 多路复用率 0x3F（64行）
    OLED_WR_Byte(0xAD, OLED_CMD);   // ★ SH1106 电荷泵命令（不同于 SSD1315）★
    OLED_WR_Byte(0x8B, OLED_CMD);   // ★ 电荷泵设置
    OLED_WR_Byte(0x32, OLED_CMD);   // ★ 电荷泵使能
    OLED_WR_Byte(0xC8, OLED_CMD);   // COM 扫描方向（上下方向）
    OLED_WR_Byte(0xD3, OLED_CMD);   // 设置显示偏移
    OLED_WR_Byte(0x00, OLED_CMD);   // 偏移量为 0
    OLED_WR_Byte(0xD5, OLED_CMD);   // 设置振荡器频率
    OLED_WR_Byte(0x80, OLED_CMD);   // 分频比
    OLED_WR_Byte(0xD9, OLED_CMD);   // 设置预充电周期
    OLED_WR_Byte(0x1F, OLED_CMD);   // 预充电值
    OLED_WR_Byte(0xDA, OLED_CMD);   // 设置 COM 引脚硬件配置
    OLED_WR_Byte(0x12, OLED_CMD);   // COM 配置
    OLED_WR_Byte(0xDB, OLED_CMD);   // 设置 VCOM 检测
    OLED_WR_Byte(0x40, OLED_CMD);   // VCOM 值
    OLED_WR_Byte(0xAF, OLED_CMD);   // 开启显示

    // 清屏（黑色）
    OLED_Clear(0);
    // 将显存内容刷新到屏幕
    OLED_Display();
}

/**
 * @brief  刷新显存到屏幕
 * @note   ★ SH1106 列地址从 0x02 开始（XLevelL = 0x02）★
 */
void OLED_Display(void)
{
    uint8_t page, col;
    // 遍历每一页（共 8 页）
    for (page = 0; page < OLED_PAGE_NUM; page++) {
        // 设置页地址（YLevel + page）
        OLED_WR_Byte(YLevel + page, OLED_CMD);
        // 设置列低地址（SH1106 从 0x02 开始）
        OLED_WR_Byte(XLevelL, OLED_CMD);
        // 设置列高地址
        OLED_WR_Byte(XLevelH, OLED_CMD);
        // 遍历该页的每一列（共 128 列）
        for (col = 0; col < OLED_WIDTH; col++) {
            // 写入该位置的显存数据
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

/**
 * @brief  清屏函数
 * @param  color : 0 表示黑色（全0），1 表示白色（全1）
 */
void OLED_Clear(uint8_t color)
{
    // 将显存全部设置为 color 对应的值（0x00 或 0xFF）
    memset(OLED_Buffer, color ? 0xFF : 0x00, sizeof(OLED_Buffer));
    // 刷新到屏幕
    OLED_Display();
}

/**
 * @brief  设置单个像素点的颜色
 * @param  x, y : 像素坐标
 * @param  color : 0 黑色，1 白色
 */
void OLED_SetPixel(uint16_t x, uint16_t y, uint8_t color)
{
    // 如果坐标超出屏幕范围，直接返回
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    // 计算该像素在显存中的索引：页号 * 宽度 + 列号
    // 注意：这里用 OLED_PAGE_NUM 作为每页的行数（8），实际上 OLED_PAGE_NUM 是页数（8），恰好等于每页行数，所以结果正确
    uint16_t index = (y / OLED_PAGE_NUM) * OLED_WIDTH + x;
    // 计算该像素在字节中的位掩码（1 << (y % 8)）
    uint8_t mask  = 1 << (y % OLED_PAGE_NUM);
    // 根据颜色设置或清除对应位
    if (color) OLED_Buffer[index] |= mask;
    else OLED_Buffer[index] &= ~mask;
}

/**
 * @brief  打开 OLED 显示（SH1106 需要开启电荷泵）
 */
void OLED_DisplayOn(void)
{
    // 开启电荷泵
    OLED_WR_Byte(0xAD, OLED_CMD);
    OLED_WR_Byte(0x8B, OLED_CMD);
    OLED_WR_Byte(0x32, OLED_CMD);
    // 开启显示
    OLED_WR_Byte(0xAF, OLED_CMD);
}

/**
 * @brief  关闭 OLED 显示
 */
void OLED_DisplayOff(void)
{
    // 关闭显示命令
    OLED_WR_Byte(0xAE, OLED_CMD);
}

/* ===== 字符显示 ===== */
/**
 * @brief  OLED 显示一个 ASCII 字符（共用 LCD 的 asc2_xxxx 字库）
 * @param  x, y     : 起始坐标
 * @param  chr      : ASCII 字符（' '~'~'）
 * @param  Char_Size: 字号（12/16/24/32）
 * @param  mode    1：白字黑底，0：黑字白底
 */
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t Char_Size, uint8_t mode)
{
    const unsigned char *pfont;     // 指向字库数据的指针
    uint8_t csize;                  // 字符点阵占用的字节数
    uint8_t t, t1, temp;            // 循环变量和临时字节
    uint8_t y0 = y;                 // 保存起始 y 坐标
    unsigned char c = chr - ' ';    // 计算字符在字库中的索引（从空格开始）

    // 如果字符超出可显示范围（' ' 到 '~'），直接返回
    if (c > 94) return;

    /* ★ 8 号：F6x8，逐行扫描 ★ */
    if (Char_Size == 8)
    {
        // 遍历 8 行
        for (uint8_t i = 0; i < 8; i++)
        {
            // 获取该行字库数据
            temp = F6x8[c][i];
            // 遍历 6 列
            for (uint8_t j = 0; j < 6; j++)
            {
                // 提取对应位（从高位到低位）
                uint8_t bit = (temp & (0x80 >> j)) ? 1 : 0;
                // 根据模式设置像素：mode=1 白字黑底，mode=0 黑字白底
                if (mode == 1)
                    OLED_SetPixel(x + j, y + i, bit);
                else
                    OLED_SetPixel(x + j, y + i, !bit);
            }
        }
        return;
    }

    /* ★ 12/16/24/32：asc2_xxxx，逐列扫描 ★ */
    switch (Char_Size)
    {
        case 12: pfont = (const unsigned char *)asc2_1206[c]; break; // 12x6 字库
        case 16: pfont = (const unsigned char *)asc2_1608[c]; break; // 16x8 字库
        case 24: pfont = (const unsigned char *)asc2_2412[c]; break; // 24x12 字库
        case 32: pfont = (const unsigned char *)asc2_3216[c]; break; // 32x16 字库
        default: return;                                            // 不支持的字号
    }

    // 计算字符点阵占用的字节数：每列 8 位，共 (Char_Size/2) 列？实际是 (Char_Size/8 + 1) * (Char_Size/2)？
    // 这里公式：csize = ((Char_Size >> 3) + ((Char_Size & 0x7) != 0 ? 1 : 0)) * (Char_Size >> 1);
    // 即：行字节数（向上取整）* 列数的一半（因为每列 8 像素，宽度为 Char_Size/2？）
    // 对于 12：行字节数 = (12/8)=1 +1 =2，列数 = 12/2=6，csize=2*6=12 字节，正确。
    // 对于 16：行字节数 = 2，列数 = 8，csize=16 字节，正确。
    // 对于 24：行字节数 = 3，列数 = 12，csize=36 字节，正确。
    // 对于 32：行字节数 = 4，列数 = 16，csize=64 字节，正确。
    csize = ((Char_Size >> 3) + (((Char_Size & 0x7) != 0) ? 1 : 0)) * (Char_Size >> 1);

    // 遍历所有字节
    for (t = 0; t < csize; t++)
    {
        temp = pfont[t];            // 取一个字节
        // 遍历字节中的 8 位
        for (t1 = 0; t1 < 8; t1++)
        {
            // 提取最高位
            uint8_t bit = (temp & 0x80) ? 1 : 0;
            // 根据模式设置像素
            if (mode == 1)
                OLED_SetPixel(x, y, bit);
            else
                OLED_SetPixel(x, y, !bit);

            temp <<= 1;             // 左移，准备下一位
            y++;                    // y 坐标下移
            // 如果一列画完（达到字符高度）
            if ((y - y0) == Char_Size)
            {
                y = y0;             // 重置 y
                x++;                // x 右移一列
                break;              // 跳出内层循环，处理下一个字节
            }
        }
    }
}


/* ====================================================================
显示字符串：循环调用 OLED_ShowChar，最后统一刷新
x：列起始横坐标  
y：行起始横坐标  
chr：要显示的数据  
Char_Size：字体大小   
mode：1：白字黑底，0：黑字白底
 ====================================================================
*/
void OLED_ShowString(uint8_t x, uint8_t y, char *chr, uint8_t Char_Size, uint8_t mode)
{
    unsigned char j = 0;            // 字符索引
    uint8_t csize;                  // 每个字符占用的水平宽度
    uint8_t x0 = x;                 // 保存起始 x 坐标，用于换行

    // 根据字号确定字符宽度（像素）
    if (Char_Size == 8)       csize = 6;    /* 6x8 */
    else if (Char_Size == 12) csize = 6;
    else if (Char_Size == 16) csize = 8;
    else if (Char_Size == 24) csize = 12;
    else if (Char_Size == 32) csize = 16;
    else return;                            // 不支持的字号

    // 遍历字符串直到结束符
    while (chr[j] != '\0')
    {
        // 如果当前 x 坐标加上字符宽度超出屏幕，则换行
        if (x + csize > OLED_WIDTH)
        {
            x = x0;                         // 回到起始 x
            y += Char_Size;                 // y 下移一行
        }
        // 显示一个字符
        OLED_ShowChar(x, y, chr[j], Char_Size, mode);
        x += csize;                         // x 右移一个字符宽度
        j++;                                // 下一个字符
    }
    // 所有字符显示完毕后，刷新到屏幕
    OLED_Display();
}

/**
 * @brief  显示无符号十进制数字
 * @param  x, y : 起始坐标
 * @param  num  : 要显示的数字
 * @param  len  : 显示位数（不足补零）
 * @param  size : 字号
 * @param  color: 颜色模式
 */
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    char str[12];                       // 临时字符串缓冲区
    sprintf(str, "%0*d", len, num);     // 格式化为指定宽度的十进制字符串，补零
    OLED_ShowString(x, y, str, size, color); // 显示字符串
}

/**
 * @brief  显示有符号十进制数字
 */
void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    char str[12];
    sprintf(str, "%+0*d", len, num);    // 带符号，补零
    OLED_ShowString(x, y, str, size, color);
}

/**
 * @brief  显示十六进制数字
 */
void OLED_ShowHexNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    char str[12];
    sprintf(str, "%0*X", len, num);     // 大写十六进制，补零
    OLED_ShowString(x, y, str, size, color);
}

/**
 * @brief  显示二进制数字
 */
void OLED_ShowBinNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    char str[33];                       // 最多 32 位 + 结束符
    uint8_t i;
    // 从低位到高位转换为字符 '0' 或 '1'，存入 str 的尾部
    for (i = 0; i < len; i++) {
        str[len - 1 - i] = (num & (1 << i)) ? '1' : '0';
    }
    str[len] = '\0';                    // 字符串结束符
    OLED_ShowString(x, y, str, size, color);
}

/* ===== 图形绘制 ===== */
/**
 * @brief  画直线（Bresenham 算法）
 */
void OLED_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color)
{
    int16_t dx = abs(x2 - x1);          // x 方向差值绝对值
    int16_t dy = abs(y2 - y1);          // y 方向差值绝对值
    int16_t sx = (x1 < x2) ? 1 : -1;    // x 步进方向
    int16_t sy = (y1 < y2) ? 1 : -1;    // y 步进方向
    int16_t err = dx - dy, e2;          // 误差项
    while (1) {
        OLED_SetPixel(x1, y1, color);   // 设置当前像素
        if (x1 == x2 && y1 == y2) break; // 到达终点，退出
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; } // 调整 x
        if (e2 < dx)  { err += dx; y1 += sy; } // 调整 y
    }
}

/**
 * @brief  画矩形（空心）
 */
void OLED_DrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color)
{
    OLED_DrawLine(x1, y1, x2, y1, color); // 上边
    OLED_DrawLine(x1, y2, x2, y2, color); // 下边
    OLED_DrawLine(x1, y1, x1, y2, color); // 左边
    OLED_DrawLine(x2, y1, x2, y2, color); // 右边
}

/**
 * @brief  画圆（空心，Bresenham 中点圆算法）
 */
void OLED_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint8_t color)
{
    int16_t x = 0, y = r, d = 3 - 2 * r; // 初始值
    while (x <= y) {
        // 利用八对称性画八个点
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

/**
 * @brief  画三角形（空心，三条线）
 */
void OLED_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color)
{
    OLED_DrawLine(x1, y1, x2, y2, color); // 边1
    OLED_DrawLine(x2, y2, x3, y3, color); // 边2
    OLED_DrawLine(x3, y3, x1, y1, color); // 边3
}

/**
 * @brief  显示位图（单色）
 * @param  x, y : 起始坐标
 * @param  width, height : 图像宽高
 * @param  bmp : 图像数据指针（每行按字节对齐）
 * @param  color : 颜色
 */
void OLED_ShowBMP(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp, uint8_t color)
{
    uint16_t i, j;
    uint8_t byte_width = (width + 7) / 8; // 每行占用的字节数
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            // 提取位图中的位
            uint8_t bit = (bmp[i * byte_width + j / 8] >> (7 - j % 8)) & 0x01;
            // 设置像素：如果 bit 为 1 则显示 color，否则显示反色
            OLED_SetPixel(x + j, y + i, bit ? color : !color);
        }
    }
}

/* ===== 填充图形 ===== */
/**
 * @brief  画实心圆
 */
void OLED_FillCircle(uint16_t x0, uint16_t y0, uint8_t r, uint8_t color)
{
    int x, y;
    for (y = -r; y <= r; y++) {
        for (x = -r; x <= r; x++) {
            // 如果在圆内，则设置像素
            if (x * x + y * y <= r * r) OLED_SetPixel(x0 + x, y0 + y, color);
        }
    }
}

/**
 * @brief  画实心三角形（使用重心坐标法判断点是否在三角形内）
 */
void OLED_FillTriangel(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color)
{
    // 计算包围盒
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
    // 遍历包围盒内的每个点
    for (uint16_t y = min_y; y <= max_y; y++) {
        for (uint16_t x = min_x; x <= max_x; x++) {
            // 计算点相对于三条边的叉积
            d1 = (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);
            d2 = (x3 - x2) * (y - y2) - (y3 - y2) * (x - x2);
            d3 = (x1 - x3) * (y - y3) - (y1 - y3) * (x - x3);
            // 如果三个叉积同号（或为零），则点在三角形内
            bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
            if (!(has_neg && has_pos)) OLED_SetPixel(x, y, color);
        }
    }
}

/**
 * @brief  OLED 显示中文字符串（共用 W25Q128 GBK 字库）
 * @param  x, y : 起始坐标
 * @param  str  : GBK 编码字符串
 * @param  size : 字号（12/16/24）
 * @param  color: 1=正常显示，0=反色显示
 */
void OLED_ShowChinese(uint8_t x, uint8_t y, char *str, uint8_t size, uint8_t color)
{
    uint8_t mat[128];                   /* ★ 32×32 = 128 字节 */
    uint8_t *pstr = (uint8_t *)str;     // 指向字符串的字节指针
    uint8_t x0 = x;                     // 保存起始 x
    uint8_t y0;                         // 保存起始 y（每列绘制时用）
    uint8_t t, t1, temp;                // 循环变量
    // 计算一个汉字点阵占用的字节数
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * size;

    // 遍历字符串
    while (*pstr != 0)
    {
        if (*pstr < 0x80)               /* ASCII 交给 OLED_ShowChar */
        {
            // 显示 ASCII 字符
            OLED_ShowChar(x, y, *pstr, size, color ? 1 : 0);
            x += size / 2;              // ASCII 宽度为字号的一半
            pstr++;
            continue;
        }

        /* GBK 汉字：从 W25Q128 读点阵 */
        // 调用 text.c 中的函数，从 NORFLASH 读取汉字点阵到 mat
        lcd_oled_text_get_hz_mat(pstr, mat, size);   /* ★ 注意函数名要和 text.h 一致 */

        /* ★ 逐列扫描解析（与 LCD 一致） */
        y0 = y;
        // 遍历所有字节（点阵数据）
        for (t = 0; t < csize; t++)
        {
            temp = mat[t];              // 取一个字节
            // 遍历字节中的 8 位
            for (t1 = 0; t1 < 8; t1++)
            {
                uint8_t bit = (temp & 0x80) ? 1 : 0; // 提取最高位

                if (color == 1)                     /* 白字黑底 */
                    OLED_SetPixel(x, y, bit);
                else                                /* 黑字白底 */
                    OLED_SetPixel(x, y, !bit);

                temp <<= 1;
                y++;
                if ((y - y0) == size)               /* 一列画完 */
                {
                    y = y0;
                    x++;
                    break;
                }
            }
        }

        pstr += 2;                      // GBK 汉字占两个字节

        // 如果 x 超出右边界，换行
        if (x > OLED_WIDTH - size) { x = x0; y += size; }
        // 如果 y 超出下边界，退出
        if (y > OLED_HEIGHT - size) break;
    }
    // 刷新到屏幕
    OLED_Display();
}

/**
 * @brief  OLED 显示中英文混排字符串
 */
void OLED_ShowString_CN(uint8_t x, uint8_t y, char *str, uint8_t size, uint8_t mode)
{
    uint8_t *pstr = (uint8_t *)str;     // 字符串指针
    uint8_t x0 = x;                     // 保存起始 x
    uint8_t mat[72];                    // 临时点阵缓冲区（足够大）
    uint8_t csize_ascii = size / 2;     // ASCII 字符宽度

    // 遍历字符串
    while (*pstr != 0)
    {
        if (*pstr < 0x80)               /* ASCII */
        {
            // 如果超出右边界，换行
            if (x + csize_ascii > OLED_WIDTH) { x = x0; y += size; }
            // 显示 ASCII 字符
            OLED_ShowChar(x, y, *pstr, size, mode);
            x += csize_ascii;
            pstr++;
        }
        else                            /* GBK 汉字 */
        {
            // 如果超出右边界，换行
            if (x + size > OLED_WIDTH) { x = x0; y += size; }
            // 读取汉字点阵
            lcd_oled_text_get_hz_mat(pstr, mat, size);
            // 逐像素绘制汉字
            for (uint8_t i = 0; i < size; i++)          // 行
            {
                for (uint8_t j = 0; j < size; j++)      // 列
                {
                    // 计算点阵中对应的字节和位
                    uint8_t byte = mat[i * (size / 8) + j / 8];
                    uint8_t bit  = (byte >> (7 - (j % 8))) & 0x01;
                    // 设置像素（注意：这里未使用 mode 参数进行反色处理，直接设为 bit）
                    OLED_SetPixel(x + j, y + i, bit);
                }
            }
            x += size;                  // 汉字宽度为 size
            pstr += 2;                  // 跳过两个字节
        }

        // 如果 y 超出下边界，退出
        if (y > OLED_HEIGHT - size) break;
    }
    // 刷新到屏幕
    OLED_Display();
}

