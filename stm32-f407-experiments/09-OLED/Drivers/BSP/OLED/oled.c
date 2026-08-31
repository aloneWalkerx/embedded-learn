#include "./BSP/OLED/oled.h"
#include "./BSP/OLED/oledfont.h"   // 字库（ASCII 和中文字体点阵）
#include "./BSP/I2C/iic.h"         // I2C 底层通信

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ====================================================================
//  显存缓冲区（1024 字节 = 128 × 64 / 8）
//  这就像一个“画板”，所有绘图操作先画在画板上，最后再整体刷到屏幕
// ====================================================================
uint8_t OLED_Buffer[OLED_WIDTH * OLED_HEIGHT / 8];

// ====================================================================
//  底层 I2C 写命令/数据（内部函数，静态的，只在当前文件使用）
//  参数 dat：要发送的字节；cmd：1 表示数据，0 表示命令
// ====================================================================
static void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    if (cmd) {
        Write_IIC_Data(dat);   // 发数据
    } else {
        Write_IIC_Command(dat); // 发命令
    }
}

// ====================================================================
//  GPIO 初始化：使能 GPIOB 时钟，把 SCL 和 SDA 设为推挽输出，并且默认拉高
// ====================================================================
static void OLED_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio_init = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();   // 开启 GPIOB 的时钟（STM32 必须）

    gpio_init.Pin  = OLED_SCL_GPIO_PIN | OLED_SDA_GPIO_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;   // 推挽输出
    gpio_init.Pull = GPIO_PULLUP;           // 内部上拉
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(OLED_SCL_GPIO_PORT, &gpio_init);

    OLED_SCL_SET();   // 初始把时钟拉高
    OLED_SDA_SET();   // 初始把数据拉高（空闲状态）
}

// ====================================================================
//  OLED 初始化：发一大堆初始化命令，让 OLED 进入正常工作状态
// ====================================================================
void OLED_Init(void)
{
    OLED_GPIO_Init();   // 先初始化硬件引脚
    delay_ms(200);      // 等待 OLED 上电稳定

    // SH1106 初始化命令序列（按数据手册来）
    OLED_WR_Byte(0xAE, OLED_CMD);   // 关闭显示（先关掉再设置）
    OLED_WR_Byte(0x02, OLED_CMD);   // 列低地址（设置起始列）
    OLED_WR_Byte(0x10, OLED_CMD);   // 列高地址
    OLED_WR_Byte(0x40, OLED_CMD);   // 起始行（从第 0 行开始）
    OLED_WR_Byte(0xB0, OLED_CMD);   // 页地址（0xB0 表示第 0 页）
    OLED_WR_Byte(0x81, OLED_CMD);   // 对比度控制命令
    OLED_WR_Byte(0xFF, OLED_CMD);   // 对比度值（最大亮度）
    OLED_WR_Byte(0xA1, OLED_CMD);   // 段重映射（左右镜像，根据硬件连接决定）
    OLED_WR_Byte(0xA6, OLED_CMD);   // 正常显示（非反色）
    OLED_WR_Byte(0xA8, OLED_CMD);   // 多路复用率（高度 64 行）
    OLED_WR_Byte(0x3F, OLED_CMD);
    OLED_WR_Byte(0xAD, OLED_CMD);   // 电荷泵使能（内部升压）
    OLED_WR_Byte(0x8B, OLED_CMD);   // 电荷泵设置
    OLED_WR_Byte(0x32, OLED_CMD);
    OLED_WR_Byte(0xC8, OLED_CMD);   // COM 扫描方向（上下颠倒，根据实际）
    OLED_WR_Byte(0xD3, OLED_CMD);   // 显示偏移
    OLED_WR_Byte(0x00, OLED_CMD);
    OLED_WR_Byte(0xD5, OLED_CMD);   // 振荡器分频
    OLED_WR_Byte(0x80, OLED_CMD);
    OLED_WR_Byte(0xD9, OLED_CMD);   // 预充电周期
    OLED_WR_Byte(0x1F, OLED_CMD);
    OLED_WR_Byte(0xDA, OLED_CMD);   // COM 引脚配置
    OLED_WR_Byte(0x12, OLED_CMD);
    OLED_WR_Byte(0xDB, OLED_CMD);   // VCOM 检测
    OLED_WR_Byte(0x40, OLED_CMD);
    OLED_WR_Byte(0xAF, OLED_CMD);   // 开启显示（最后开起来）

    OLED_Clear(0);      // 清屏（全黑）
    OLED_Display();     // 刷新一次，让屏幕显示黑色
}

// ====================================================================
//  刷新显存到屏幕：把 OLED_Buffer 里的数据全部发送给 OLED
//  因为 OLED 是分页（每页 8 行）的，所以逐页发送
// ====================================================================
void OLED_Display(void)
{
    uint8_t page, col;

    for (page = 0; page < OLED_PAGE_NUM; page++) {
        OLED_WR_Byte(YLevel + page, OLED_CMD);      // 设置页地址 0xB0~0xB7（第几页）
        OLED_WR_Byte(XLevelL, OLED_CMD);            // 列低地址（固定 0x02）
        OLED_WR_Byte(XLevelH, OLED_CMD);            // 列高地址（固定 0x10）

        for (col = 0; col < OLED_WIDTH; col++) {
            // 从缓冲区取出对应位置的数据，发送给 OLED（数据模式）
            OLED_WR_Byte(OLED_Buffer[page * OLED_WIDTH + col], OLED_DATA);
        }
    }
}

// ====================================================================
//  清屏：把整个缓冲区填充为 0x00（黑色）或 0xFF（白色），然后刷新
// ====================================================================
void OLED_Clear(uint8_t color)
{
    memset(OLED_Buffer, color ? 0xFF : 0x00, sizeof(OLED_Buffer));
    OLED_Display();   // 立即刷新
}

// ====================================================================
//  画点：在 (x, y) 处画一个点（color=1 亮，0 暗）
//  原理：计算这个点属于哪一页（y/8），哪一列（x），以及在该字节中的哪一位（y%8）
// ====================================================================
void OLED_SetPixel(uint16_t x, uint16_t y, uint8_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) {
        return;   // 超出屏幕范围就忽略
    }

    uint16_t index = (y / OLED_PAGE_NUM) * OLED_WIDTH + x;   // 计算在缓冲区中的索引
    uint8_t mask  = 1 << (y % OLED_PAGE_NUM);               // 计算该点对应字节的哪一位（0~7）

    if (color) {
        OLED_Buffer[index] |= mask;    // 置 1（点亮）
    } else {
        OLED_Buffer[index] &= ~mask;   // 置 0（熄灭）
    }
}

// ====================================================================
//  显示开关（通过命令控制 OLED 电源）
// ====================================================================
void OLED_DisplayOn(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);   // 电荷泵命令
    OLED_WR_Byte(0x14, OLED_CMD);   // 使能电荷泵
    OLED_WR_Byte(0xAF, OLED_CMD);   // 开启显示
}

void OLED_DisplayOff(void)
{
    OLED_WR_Byte(0x8D, OLED_CMD);
    OLED_WR_Byte(0x10, OLED_CMD);   // 禁用电荷泵
    OLED_WR_Byte(0xAE, OLED_CMD);   // 关闭显示
}

// ====================================================================
//  显示单个字符（ASCII）
//  x, y 起始坐标，chr 字符，Char_Size 字体大小（8 或 16），mode 颜色（1亮0暗）
// ====================================================================
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t Char_Size, uint8_t mode)
{
    unsigned char c = 0, i = 0, tmp, j = 0;

    c = chr - ' ';   // 因为字库从空格（ASCII 32）开始，所以减去偏移
    if (x > OLED_WIDTH - 1) {
        x = 0;
        y += 2;      // 换行（简单处理）
    }

    if (Char_Size == 16) {
        // 8x16 字体（每个字符 16 字节）
        for (i = 0; i < 16; i++) {
            tmp = mode ? F8X16[c * 16 + i] : ~(F8X16[c * 16 + i]); // 如果需要反色，取反
            for (j = 0; j < 8; j++) {
                // 逐位判断，调用 OLED_SetPixel 画点
                OLED_SetPixel(x + j, y + i, (tmp & (0x80 >> j)) ? 1 : 0);
            }
        }
    } else if (Char_Size == 8) {
        // 6x8 字体（每个字符 8 字节）
        for (i = 0; i < 8; i++) {
            tmp = mode ? F6x8[c][i] : ~(F6x8[c][i]);
            for (j = 0; j < 8; j++) {
                OLED_SetPixel(x + j, y + i, (tmp & (0x80 >> j)) ? 1 : 0);
            }
        }
    }
    // 其他尺寸不做处理
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
    unsigned char j = 0;
    uint8_t csize;

    if (Char_Size == 16) {
        csize = 8;      // 8x16 字体每个字符宽度 8 像素
    } else if (Char_Size == 8) {
        csize = 6;      // 6x8 字体宽度 6 像素
    } else {
        return;
    }

    while (chr[j] != '\0') {
        OLED_ShowChar(x, y, chr[j], Char_Size, mode);
        x += csize;            // 移到下一个字符位置
        if (x > 120) {         // 如果接近右边界，换行
            x = 0;
            y += Char_Size;
        }
        j++;
    }

    OLED_Display();     // 整串画完后统一刷新（提高效率）
}

// ====================================================================
//  数字显示（无符号、有符号、十六进制、二进制）
//  都是利用 sprintf 转换成字符串，再调用 OLED_ShowString
// ====================================================================
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    char str[12];
    sprintf(str, "%0*d", len, num);   // 格式化：固定长度，不足补 0
    OLED_ShowString(x, y, str, size, color);
}

void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    char str[12];
    sprintf(str, "%+0*d", len, num);  // 带符号
    OLED_ShowString(x, y, str, size, color);
}

void OLED_ShowHexNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    char str[12];
    sprintf(str, "%0*X", len, num);   // 大写十六进制
    OLED_ShowString(x, y, str, size, color);
}

void OLED_ShowBinNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t color)
{
    char str[33];
    uint8_t i;
    for (i = 0; i < len; i++) {
        str[len - 1 - i] = (num & (1 << i)) ? '1' : '0';   // 手动转二进制
    }
    str[len] = '\0';
    OLED_ShowString(x, y, str, size, color);
}

// ====================================================================
//  图形绘制：画线、矩形、圆、三角形（Bresenham 算法等）
// ====================================================================
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
    // 中点画圆算法
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

        if (d < 0) {
            d = d + 4 * x + 6;
        } else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void OLED_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color)
{
    OLED_DrawLine(x1, y1, x2, y2, color);
    OLED_DrawLine(x2, y2, x3, y3, color);
    OLED_DrawLine(x3, y3, x1, y1, color);
}

// ====================================================================
//  显示 BMP 单色图片（逐行逐像素画点）
// ====================================================================
void OLED_ShowBMP(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp, uint8_t color)
{
    uint16_t i, j;
    uint8_t byte_width = (width + 7) / 8;   // 每行需要几个字节（宽度不足 8 的补足）

    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            uint8_t bit = (bmp[i * byte_width + j / 8] >> (7 - j % 8)) & 0x01;
            OLED_SetPixel(x + j, y + i, bit ? color : !color);
        }
    }
}

// ====================================================================
//  填充圆形（暴力遍历矩形区域，判断距离）
// ====================================================================
void OLED_FillCircle(uint16_t x0, uint16_t y0, uint8_t r, uint8_t color)
{
    int x, y;
    for (y = -r; y <= r; y++) {
        for (x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                OLED_SetPixel(x0 + x, y0 + y, color);
            }
        }
    }
}

// ====================================================================
//  填充三角形（叉积法判断点是否在三角形内）
// ====================================================================
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

            if (!(has_neg && has_pos)) {   // 同号或零，说明在三角形内或边界
                OLED_SetPixel(x, y, color);
            }
        }
    }
}

// ====================================================================
//  显示中文字符串（顺序索引模式）
//  注意：字库必须按字符串顺序排列，这里只演示了 16/24/32 点阵
// ====================================================================
void OLED_ShowChinese(uint8_t x, uint8_t y, char *str, uint8_t size, uint8_t color)
{
    uint8_t  j, k, tmp;
    uint16_t num;
    uint8_t char_index = 0;
    
    while (*str) {
        if (size == 16) {
            num = sizeof(cfont16) / sizeof(typFNT_GB16);
            if (char_index < num) {
                for (j = 0; j < 32; j++) {   // 16x16 点阵有 32 字节
                    tmp = color ? cfont16[char_index].Msk[j] : ~cfont16[char_index].Msk[j];
                    for (k = 0; k < 8; k++) {
                        if (tmp & (0x80 >> k)) {
                            OLED_SetPixel(x + (j % 2) * 8 + k, y + j / 2, 1);
                        } else {
                            OLED_SetPixel(x + (j % 2) * 8 + k, y + j / 2, 0);
                        }
                    }
                }
            }
            x += size;
            char_index++;
            
        } else if (size == 24) {
            // 类似，但每字 72 字节，每行 3 个字节
            num = sizeof(cfont24) / sizeof(typFNT_GB24);
            if (char_index < num) {
                for (j = 0; j < 72; j++) {
                    tmp = color ? cfont24[char_index].Msk[j] : ~cfont24[char_index].Msk[j];
                    for (k = 0; k < 8; k++) {
                        if (tmp & (0x80 >> k)) {
                            OLED_SetPixel(x + (j % 3) * 8 + k, y + j / 3, 1);
                        } else {
                            OLED_SetPixel(x + (j % 3) * 8 + k, y + j / 3, 0);
                        }
                    }
                }
            }
            x += size;
            char_index++;
            
        } else if (size == 32) {
            // 32x32，每字 128 字节，每行 4 个字节
            num = sizeof(cfont32) / sizeof(typFNT_GB32);
            if (char_index < num) {
                for (j = 0; j < 128; j++) {
                    tmp = color ? cfont32[char_index].Msk[j] : ~cfont32[char_index].Msk[j];
                    for (k = 0; k < 8; k++) {
                        if (tmp & (0x80 >> k)) {
                            OLED_SetPixel(x + (j % 4) * 8 + k, y + j / 4, 1);
                        } else {
                            OLED_SetPixel(x + (j % 4) * 8 + k, y + j / 4, 0);
                        }
                    }
                }
            }
            x += size;
            char_index++;
        }
        
        str += 2;  // 中文 GBK 编码占两个字节
        if (x > OLED_WIDTH - size) {
            x = 0;
            y += size;
        }
    }
}
