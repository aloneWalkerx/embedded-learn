#include "stdlib.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/LCD/lcd-font.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"



//SRAM句柄
SRAM_HandleTypeDef g_sram_handle = {0}; 


/* 绘制LCD时的背景色 */
uint32_t g_back_color = 0xFFFF;


/* LCD重要参数 */
_lcd_dev lcddev;

/**
 * @brief   LCD写数据
 * @param   data: 要写入的数据
 * @retval  无
 */
void lcd_wr_data(volatile uint16_t data)
{
    data = data;
    LCD->LCD_RAM = data;
}


/**
 * @brief   LCD写寄存器编号或地址
 * @param   regno: 寄存器编号或地址
 * @retval  无
 */
void lcd_wr_regno(volatile uint16_t regno)
{
    regno = regno;
    LCD->LCD_REG = regno;
}

/**
 * @brief   LCD写寄存器
 * @param   regno: 寄存器编号
 * @param   data : 要写入的数据
 * @retval  无
 */
void lcd_write_reg(uint16_t regno, uint16_t data)
{
    LCD->LCD_REG = regno;
    LCD->LCD_RAM = data;
}


/**
 * @brief   LCD读数据
 * @param   无
 * @retval  读取到的数据
 */
static uint16_t lcd_rd_data(void)
{
    volatile uint16_t ram;
    
    ram = LCD->LCD_RAM;
    
    return ram;
}



/**
 * @brief   LCD延时函数
 * @note    仅用于部分在-O1时间优化时需要设置的地方
 * @param   t: 延时的数值
 * @retval  无
 */
static void lcd_opt_delay(uint32_t i)
{
    /* 使用AC6时空循环可能被优化，可使用while(1) __asm volatile(""); */
    while (i--);
}


/**
 * @brief   准备写GRAM
 * @param   无
 * @retval  无
 */
void lcd_write_ram_prepare(void)
{
    LCD->LCD_REG = lcddev.wramcmd;
}


/**
 * @brief   读取个某点的颜色值
 * @param   x: 指定点的X坐标
 * @param   y: 指定点的Y坐标
 * @retval  指定点的颜色（32位颜色，方便兼容LTDC）
 */
uint32_t lcd_read_point(uint16_t x, uint16_t y)
{
    uint16_t r;
    uint16_t g;
    uint16_t b;
    
    if ((x >= lcddev.width) || (y >= (lcddev.height)))          /* 判断点的坐标是否合法 */
    {
        return 0;
    }
    
    lcd_set_cursor(x, y);                                       /* 设置光标 */
    
   
    /* 5510/9341/5310/7789第一次读出RG数据，R在前，G在后，各占8位 */
    lcd_opt_delay(2);
    b = lcd_rd_data();                                          /* 第二次读出BR（R是下一个像素的颜色数据）数据，B在前，R在后，各占8位 */
    g = (r & 0xFF) << 8;
    
    return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));  /* RGB565 */
}


/**
 * @brief   LCD开启显示
 * @param   无
 * @retval  无
 */
void lcd_display_on(void)
{
    if (lcddev.id == 0x5510)    /* 5510 */
    {
        lcd_wr_regno(0x2900);
    }
    else                        /* 9341/5310/1963/7789 */
    {
        lcd_wr_regno(0x29);
    }
}


/**
 * @brief   LCD关闭显示
 * @param   无
 * @retval  无
 */
void lcd_display_off(void)
{
    if (lcddev.id == 0x5510)    /* 5510 */
    {
        lcd_wr_regno(0x2800);
    }
    else                        /* 9341/5310/1963/7789 */
    {
        lcd_wr_regno(0x28);
    }
}


/**
 * @brief   设置光标位置（对RGB屏无效）
 * @param   x: 光标的X坐标
 * @param   y: 光标的Y坐标
 * @retval  无
 */
void lcd_set_cursor(uint16_t x, uint16_t y)
{
        lcd_wr_regno(lcddev.setxcmd);
        lcd_wr_data(x >> 8);
        lcd_wr_data(x & 0xFF);
        lcd_wr_regno(lcddev.setycmd);
        lcd_wr_data(y >> 8);
        lcd_wr_data(y & 0xFF);
    
}

/**
 * @brief   设置LCD的自动扫描方向（对RGB屏无效）
 * @note    9341/5310/5510/1963/7789等IC已经实际测试
 *          注意:其他函数可能会受到此函数设置的影响（尤其是9341），
 *          所以，一般设置为L2R_U2D即可，如果设置为其他扫描方式，可能导致显示不正常。
 * @param   dir: LCD扫描方向
 *   @arg   L2R_U2D: 从左到右，从上到下
 *   @arg   L2R_D2U: 从左到右，从下到上
 *   @arg   R2L_U2D: 从右到左，从上到下
 *   @arg   R2L_D2U: 从右到左，从下到上
 *   @arg   U2D_L2R: 从上到下，从左到右
 *   @arg   U2D_R2L: 从上到下，从右到左
 *   @arg   D2U_L2R: 从下到上，从左到右
 *   @arg   D2U_R2L: 从下到上，从右到左
 * @retval  无
 */
void lcd_scan_dir(uint8_t dir)
{
    uint16_t regval = 0;
    uint16_t dirreg;
    uint16_t temp;
    
    /* 横屏时，1963不改变扫描方向，其他IC改变扫描方向
     * 竖屏时，1963改变扫描方向，其他IC不改变扫描方向
     */
    if (((lcddev.dir == 1) && (lcddev.id != 0x1963)) || ((lcddev.dir == 0) && (lcddev.id == 0x1963)))
    {
        switch (dir)   /* 方向转换 */
        {
            case L2R_U2D:
            {
                dir = D2U_L2R;
                break;
            }
            case L2R_D2U:
            {
                dir = D2U_R2L;
                break;
            }
            case R2L_U2D:
            {
                dir = U2D_L2R;
                break;
            }
            case R2L_D2U:
            {
                dir = U2D_R2L;
                break;
            }
            case U2D_L2R:
            {
                dir = L2R_D2U;
                break;
            }
            case U2D_R2L:
            {
                dir = L2R_U2D;
                break;
            }
            case D2U_L2R:
            {
                dir = R2L_D2U;
                break;
            }
            case D2U_R2L:
            {
                dir = R2L_U2D;
                break;
            }
        }
    }
    
    /* 根据扫描方向设置0x36或0x3600寄存器bit5~7位的值 */
    switch (dir)
    {
        case L2R_U2D:
        {
            regval |= (0 << 7) | (0 << 6) | (0 << 5);
            break;
        }
        case L2R_D2U:
        {
            regval |= (1 << 7) | (0 << 6) | (0 << 5);
            break;
        }
        case R2L_U2D:
        {
            regval |= (0 << 7) | (1 << 6) | (0 << 5);
            break;
        }
        case R2L_D2U:
        {
            regval |= (1 << 7) | (1 << 6) | (0 << 5);
            break;
        }
        case U2D_L2R:
        {
            regval |= (0 << 7) | (0 << 6) | (1 << 5);
            break;
        }
        case U2D_R2L:
        {
            regval |= (0 << 7) | (1 << 6) | (1 << 5);
            break;
        }
        case D2U_L2R:
        {
            regval |= (1 << 7) | (0 << 6) | (1 << 5);
            break;
        }
        case D2U_R2L:
        {
            regval |= (1 << 7) | (1 << 6) | (1 << 5);
            break;
        }
    }
    
    dirreg = 0x36;  /* 对绝大部分驱动IC, 由0x36寄存器控制 */
    
  
    /* 9341/7789/7796要设置BGR位 */
    if ((lcddev.id == 0x9341) || (lcddev.id == 0x7789) || (lcddev.id == 0x7796))
    {
        regval |= 0x08;
    }
    
    lcd_write_reg(dirreg, regval);
    
    if (lcddev.id != 0x1963)                    /* 1963不用做坐标处理 */
    {
        if (regval & 0x20)
        {
            if (lcddev.width < lcddev.height)   /* 交换X和Y */
            {
                temp = lcddev.width;
                lcddev.width = lcddev.height;
                lcddev.height = temp;
            }
        }
        else
        {
            if (lcddev.width > lcddev.height)   /* 交换X和Y */
            {
                temp = lcddev.width;
                lcddev.width = lcddev.height;
                lcddev.height = temp;
            }
        }
    }
    
    
        /* 9341/5310/1963/7789 */
    
        lcd_wr_regno(lcddev.setxcmd);
        lcd_wr_data(0);
        lcd_wr_data(0);
        lcd_wr_data((lcddev.width - 1) >> 8);
        lcd_wr_data((lcddev.width - 1) & 0xFF);
        lcd_wr_regno(lcddev.setycmd);
        lcd_wr_data(0);
        lcd_wr_data(0);
        lcd_wr_data((lcddev.height - 1) >> 8);
        lcd_wr_data((lcddev.height - 1) & 0xFF);
    
}



/**
 * @brief   画点
 * @param   x: 点的X坐标
 * @param   y: 点的Y坐标
 * @param   color: 点的颜色（32位颜色，方便兼容LTDC）
 * @retval  无
 */
void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
    lcd_set_cursor(x, y);       /* 设置光标位置 */
    lcd_write_ram_prepare();    /* 开始写入GRAM */
    LCD->LCD_RAM = color;
}


/**
 * @brief   设置SSD1963背光亮度
 * @param   pwm: 背光等级，范围0~100（数值越大越亮）
 * @retval  无
 */
void lcd_ssd_backlight_set(uint8_t pwm)
{
    lcd_wr_regno(0xBE);         /* 配置PWM输出 */
    lcd_wr_data(0x05);          /* 设置PWM频率 */
    lcd_wr_data(pwm * 2.55);    /* 设置PWM占空比 */
    lcd_wr_data(0x01);          /* 设置C */
    lcd_wr_data(0xFF);          /* 设置D */
    lcd_wr_data(0x00);          /* 设置E */
    lcd_wr_data(0x00);          /* 设置F */
}

/**
 * @brief   设置LCD显示方向
 * @param   dir: LCD显示方向
 *   @arg   0: 竖屏
 *   @arg   1: 横屏
 * @retval  无
 */
void lcd_display_dir(uint8_t dir)
{
    lcddev.dir = dir;
    
    if (dir == 0)           /* 竖屏 */
    {
        lcddev.width = 240;
        lcddev.height = 320;

        
       /* 其他IC, 包括: 9341 / 5310 / 7789/7796/9806等IC */
        
            lcddev.wramcmd = 0X2C;
            lcddev.setxcmd = 0X2A;
            lcddev.setycmd = 0X2B;

    }

    lcd_scan_dir(DFT_SCAN_DIR);         /* 设置LCD为默认扫描方向 */
}

/**
 * @brief   设置窗口（对RGB屏无效）
 * @note    会自动设置画点坐标到窗口左上角(sx,sy)
 * @param   sx    : 窗口起始X坐标
 * @param   sy    : 窗口起始Y坐标
 * @param   width : 窗口宽度，需大于0
 * @param   height: 窗口高度，需大于0
 *  @note   窗口大小 = width * height
 * @retval  无
 */
void lcd_set_window(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
    uint16_t twidth;
    uint16_t theight;
    
    twidth = sx + width - 1;
    theight = sy + height - 1;
    
     /* 9341/5310/1963（横屏）/7789/9806 */
    
        lcd_wr_regno(lcddev.setxcmd);
        lcd_wr_data(sx >> 8);
        lcd_wr_data(sx & 0xFF);
        lcd_wr_data(twidth >> 8);
        lcd_wr_data(twidth & 0xFF);
        lcd_wr_regno(lcddev.setycmd);
        lcd_wr_data(sy >> 8);
        lcd_wr_data(sy & 0xFF);
        lcd_wr_data(theight >> 8);
        lcd_wr_data(theight & 0xFF);
    
}

/**
 * @brief   清屏
 * @param   color: 清屏的颜色
 * @retval  无
 */
void lcd_clear(uint16_t color)
{
    uint32_t index;
    uint32_t totalpoint;
    
    totalpoint = lcddev.width * lcddev.height;  /* 计算总像素数量 */
    lcd_set_cursor(0x00, 0x0000);               /* 设置光标位置 */
    lcd_write_ram_prepare();                    /* 开始写入GRAM */
    for (index=0; index<totalpoint; index++)
    {
        LCD->LCD_RAM = color;
    }
}

/**
 * @brief   在指定区域内填充单个颜色
 * @param   sx    : 指定区域的起始X坐标
 * @param   sy    : 指定区域的起始Y坐标
 * @param   ex    : 指定区域的结束X坐标
 * @param   ey    : 指定区域的结束Y坐标
 * @param   color : 要填充的颜色（32位颜色，方便兼容LTDC）
 *  @note   指定区域的大小 = (ex - sx + 1) * (ey - sy + 1)
 * @retval  无
 */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    uint16_t i;
    uint16_t j;
    uint16_t xlen;
    
    xlen = ex - sx + 1;
    for (i=sy; i<=ey; i++)
    {
        lcd_set_cursor(sx, i);      /* 设置光标位置 */
        lcd_write_ram_prepare();    /* 开始写入GRAM */
        for (j=0; j<xlen; j++)
        {
            LCD->LCD_RAM = color;
        }
    }
}


/**
 * @brief   在指定区域内填充指定颜色块
 * @param   sx    : 指定区域的起始X坐标
 * @param   sy    : 指定区域的起始Y坐标
 * @param   ex    : 指定区域的结束X坐标
 * @param   ey    : 指定区域的结束Y坐标
 * @param   color : 指定颜色数组的首地址
 *  @note   指定区域的大小 = (ex - sx + 1) * (ey - sy + 1)
 * @retval  无
 */
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    uint16_t height;
    uint16_t width;
    uint16_t i;
    uint16_t j;
    
    width = ex - sx + 1;            /* 计算指定区域的宽度 */
    height = ey - sy + 1;           /* 计算指定区域的高度 */
    for (i=0; i<height; i++)
    {
        lcd_set_cursor(sx, sy + i); /* 设置光标位置 */
        lcd_write_ram_prepare();    /* 开始写入GRAM */
        for (j=0; j<width; j++)
        {
            LCD->LCD_RAM = color[i * width + j];
        }
    }
}

/**
 * @brief   画线
 * @param   x1   : 线的起始X坐标
 * @param   y1   : 线的起始Y坐标
 * @param   x2   : 线的结束X坐标
 * @param   y2   : 线的结束Y坐标
 * @param   color: 线的颜色
 * @retval  无
 */
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t;
    int xerr = 0;
    int yerr = 0;
    int delta_x;
    int delta_y;
    int distance;
    int incx;
    int incy;
    int row;
    int col;
    
    /* 计算坐标增量 */
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    
    row = x1;
    col = y1;
    
    /* 设置X单步方向 */
    if (delta_x > 0)
    {
        incx = 1;
    }
    else if (delta_x == 0)
    {
        incx = 0;
    }
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }
    
    /* 设置Y单步方向 */
    if (delta_y > 0)
    {
        incy = 1;
    }
    else if (delta_y == 0)
    {
        incy = 0;
    }
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }
    
    /* 选取基本增量坐标轴 */
    if (delta_x > delta_y)
    {
        distance = delta_x;
    }
    else
    {
        distance = delta_y;
    }
    
    for (t=0; t<=(distance+1); t++)
    {
        lcd_draw_point(row, col, color);
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            row += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            col += incy;
        }
    }
}

/**
 * @brief   画水平线
 * @param   x    : 线的起始X坐标
 * @param   y    : 线的起始Y坐标
 * @param   len  : 线的长度
 * @param   color: 线的颜色
 * @retval  无
 */
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    if ((len == 0) || (x > lcddev.width) || (y > lcddev.height))
    {
        return;
    }
    
    lcd_fill(x, y, x + len - 1, y, color);
}


/**
 * @brief   画矩形
 * @param   x1   : 矩形左上角X坐标
 * @param   y1   : 矩形左上角Y坐标
 * @param   x2   : 矩形右下角X坐标
 * @param   y2   : 矩形右下角Y坐标
 * @param   color: 矩形的颜色
 * @retval  无
 */
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    lcd_draw_line(x1, y1, x2, y1, color);
    lcd_draw_line(x1, y1, x1, y2, color);
    lcd_draw_line(x1, y2, x2, y2, color);
    lcd_draw_line(x2, y1, x2, y2, color);
}

/**
 * @brief   画圆
 * @param   x0   : 圆心的X坐标
 * @param   y0   : 圆心的Y坐标
 * @param   r    : 圆的半径
 * @param   color: 圆的颜色
 * @retval  无
 */
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a;
    int b;
    int di;
    
    a = 0;
    b = r;
    di = 3 - (r << 1);  /* 判断下个点位置的标志 */
    
    while (a <= b)      /* 使用Bresenham算法画圆 */
    {
        lcd_draw_point(x0 + a, y0 - b, color);
        lcd_draw_point(x0 + b, y0 - a, color);
        lcd_draw_point(x0 + b, y0 + a, color);
        lcd_draw_point(x0 + a, y0 + b, color);
        lcd_draw_point(x0 - a, y0 + b, color);
        lcd_draw_point(x0 - b, y0 + a, color);
        lcd_draw_point(x0 - a, y0 - b, color);
        lcd_draw_point(x0 - b, y0 - a, color);
        a++;
        if (di < 0)
        {
            di += 4 * a + 6;
        }
        else
        {
            di += 10 + 4 * (a - b);
            b--;
        }
    }
}

/**
 * @brief   画实心圆
 * @param   x    : 圆心的X坐标
 * @param   y    : 圆心的Y坐标
 * @param   r    : 圆的半径
 * @param   color: 圆的颜色
 * @retval  无
 */
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
    uint32_t i;
    uint32_t imax;
    uint32_t sqmax;
    uint32_t xr;
    
    imax = ((uint32_t)r * 707) / 1000 + 1;
    sqmax = (uint32_t)r * (uint32_t)r + (uint32_t)r / 2;
    xr = r;
    
    lcd_draw_hline(x - r, y, 2 * r, color);
    for (i=1; i<=imax; i++)
    {
        if ((i * i + xr * xr) > sqmax)
        {
            if (xr > imax)
            {
                lcd_draw_hline (x - i + 1, y + xr, 2 * (i - 1), color);
                lcd_draw_hline (x - i + 1, y - xr, 2 * (i - 1), color);
            }
            xr--;
        }
        lcd_draw_hline(x - xr, y + i, 2 * xr, color);
        lcd_draw_hline(x - xr, y - i, 2 * xr, color);
    }
}

/**
 * @brief   在指定位置显示一个字符
 * @param   x    : 指定位置的X坐标
 * @param   y    : 指定位置的Y坐标
 * @param   chr  : 要显示的字符，范围：' '~'~'
 * @param   size : 字体
 *   @arg   12: 12*12 ASCII字符
 *   @arg   16: 16*16 ASCII字符
 *   @arg   24: 24*24 ASCII字符
 *   @arg   32: 32*32 ASCII字符
 * @param   mode : 显示模式
 *   @arg   0: 非叠加方式
 *   @arg   1: 叠加方式
 * @param   color: 字符的颜色
 * @retval  无
 */
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t csize;
    uint8_t *pfont;
    uint16_t y0;
    uint8_t t;
    uint8_t t1;
    uint8_t temp;
    
    csize = ((size >> 3) + (((size & 0x7) != 0) ? 1 : 0)) * (size >> 1);    /* 计算所选字体对应一个字符所占的字节数 */
    chr -= ' ';                                                             /* 计算偏移后的值，因为字库是从空格开始的 */
    
    switch (size)
    {
        case 12:                                                            /* 12*12 ASCII字符 */
        {
            pfont = (uint8_t *)asc2_1206[chr];
            break;
        }
        case 16:                                                            /* 16*16 ASCII字符 */
        {
            pfont = (uint8_t *)asc2_1608[chr];
            break;
        }
        case 24:                                                            /* 24*24 ASCII字符 */
        {
            pfont = (uint8_t *)asc2_2412[chr];
            break;
        }
        case 32:                                                            /* 32*32 ASCII字符 */
        {
            pfont = (uint8_t *)asc2_3216[chr];
            break;
        }
        default:
        {
            return;
        }
    }
    
    y0 = y;
    for (t=0; t<csize; t++)
    {
        temp = pfont[t];                                                    /* 获取字符的点阵数据 */
        for (t1=0; t1<8; t1++)                                              /* 遍历一个字节的8个位 */
        {
            if ((temp & 0x80) != 0)                                         /* 需要显示的有效点 */
            {
                lcd_draw_point(x, y, color);                                /* 以字符颜色绘制这个点 */
            }
            else if (mode == 0)                                             /* 不需要显示的无效点 */
            {
                lcd_draw_point(x, y, g_back_color);                         /* 绘制背景色 */
            }
            
            temp <<= 1;                                                     /* 移位至下一个位 */
            y++;
            if (y >= lcddev.height)                                         /* 判断Y坐标是否超出显示区域 */
            {
                return;
            }
            if ((y - y0) == size)                                           /* 本行绘制完成 */
            {
                y = y0;                                                     /* Y坐标复位 */
                x++;                                                        /* 下一行 */
                if (x >= lcddev.width)                                      /* 判断X坐标是否超出显示区域 */
                {
                    return;
                }
                break;
            }
        }
    }
}

/**
 * @brief   平方函数
 * @param   m: 底数
 * @param   n: 指数
 * @retval  m^n
 */
static uint32_t lcd_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    
    while (n--)
    {
        result *= m;
    }
    
    return result;
}


/**
 * @brief   显示len个数字
 * @param   x    : 起始X坐标
 * @param   y    : 起始Y坐标
 * @param   num  : 数值，范围：0~2^32
 * @param   len  : 显示数字的位数
 * @param   size : 字体
 *   @arg   12: 12*12 ASCII字符
 *   @arg   16: 16*16 ASCII字符
 *   @arg   24: 24*24 ASCII字符
 *   @arg   32: 32*32 ASCII字符
 * @param   color: 数字的颜色
 * @retval  无
 */
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t t;
    uint8_t temp;
    uint8_t enshow = 0;
    
    for (t=0; t<len; t++)                                                   /* 按总显示位数循环 */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;                       /* 获取对应位的数字 */
        if((enshow == 0) && (t < (len - 1)))                                /* 没有使能显示，且还有位要显示 */
        {
            if (temp == 0)
            {
                lcd_show_char(x + (size >> 1) * t, y, ' ', size, 0, color); /* 显示空格，占位 */
                continue;                                                   /* 继续下一个位 */
            }
            else
            {
                enshow = 1;                                                 /* 使能显示 */
            }
        }
        
        lcd_show_char(x + (size >> 1) * t, y, temp + '0', size, 0, color);  /* 显示字符 */
    }
}

/**
 * @brief   扩展显示len个数字（显示高位0）
 * @param   x    : 起始X坐标
 * @param   y    : 起始Y坐标
 * @param   num  : 数值，范围：0~2^32
 * @param   len  : 显示数字的位数
 * @param   size : 字体
 *   @arg   12: 12*12 ASCII字符
 *   @arg   16: 16*16 ASCII字符
 *   @arg   24: 24*24 ASCII字符
 *   @arg   32: 32*32 ASCII字符
 * @param   mode : 显示模式
 *   @arg   0: 非叠加方式
 *   @arg   1: 叠加方式
 * @param   color: 数字的颜色
 * @retval  无
 */
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t t;
    uint8_t temp;
    uint8_t enshow = 0;
    
    for (t=0; t<len; t++)                                                                   /* 按总显示位数循环 */
    {
        temp = (num / lcd_pow(10, len - t - 1)) % 10;                                       /* 获取对应位的数字 */
        if((enshow == 0) && (t < (len - 1)))                                                /* 没有使能显示，且还有位要显示 */
        {
            if (temp == 0)
            {
                if ((mode & 0x80) != 0)                                                     /* 高位需要填充0 */
                {
                    lcd_show_char(x + (size >> 1) * t, y, '0', size, mode & 0x01, color);   /* 显示0，占位 */
                }
                else
                {
                    lcd_show_char(x + (size >> 1) * t, y, ' ', size, mode & 0x01, color);   /* 显示空格，占位 */
                }
                continue;                                                                   /* 继续下一个位 */
            }
            else
            {
                enshow = 1;                                                                 /* 使能显示 */
            }
        }
        
        lcd_show_char(x + (size >> 1) * t, y, temp + '0', size, mode & 0x01, color);        /* 显示字符 */
    }
}


/**
 * @brief   显示字符串
 * @param   x     : 起始X坐标
 * @param   y     : 起始Y坐标
 * @param   width : 显示区域宽度
 * @param   height: 显示区域高度
 * @param   size  : 字体
 *   @arg   12: 12*12 ASCII字符
 *   @arg   16: 16*16 ASCII字符
 *   @arg   24: 24*24 ASCII字符
 *   @arg   32: 32*32 ASCII字符
 * @param   *p    : 字符串指针
 * @param   color : 字符串的颜色
 * @retval  无
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0;
    
    x0 = x;
    width += x;
    height += y;
    while ((*p <= '~') && (*p >= ' '))              /* 判断是否为非法字符 */
    {
        if (x >= width)                             /* 宽度越界 */
        {
            x = x0;                                 /* 换行 */
            y += size;
        }
        
        if (y >= height)                            /* 高度越界 */
        {
            break;                                  /* 退出 */
        }
        
        lcd_show_char(x, y, *p, size, 0, color);    /* 显示一个字符 */
        x += (size >> 1);                           /* ASCII字符宽度为高度的一半 */
        p++;
    }
}

/**
 * @brief       ST7789寄存器初始化
 * @param       无
 * @retval      无
 */
void lcd_ex_st7789_reginit(void)
{
    lcd_wr_regno(0x11);
    delay_ms(120);
    lcd_wr_regno(0x36);
    lcd_wr_data(0x00);
    lcd_wr_regno(0x3A);
    lcd_wr_data(0x05);
    lcd_wr_regno(0xB2);
    lcd_wr_data(0x0C);
    lcd_wr_data(0x0C);
    lcd_wr_data(0x00);
    lcd_wr_data(0x33);
    lcd_wr_data(0x33);
    lcd_wr_regno(0xB7);
    lcd_wr_data(0x35);
    lcd_wr_regno(0xBB);
    lcd_wr_data(0x32);
    lcd_wr_regno(0xC0);
    lcd_wr_data(0x0C);
    lcd_wr_regno(0xC2);
    lcd_wr_data(0x01);
    lcd_wr_regno(0xC3);
    lcd_wr_data(0x10);
    lcd_wr_regno(0xC4);
    lcd_wr_data(0x20);
    lcd_wr_regno(0xC6);
    lcd_wr_data(0x0F);
    lcd_wr_regno(0xD0);
    lcd_wr_data(0xA4);
    lcd_wr_data(0xA1);
    lcd_wr_regno(0xE0);
    lcd_wr_data(0xD0);
    lcd_wr_data(0x00);
    lcd_wr_data(0x02);
    lcd_wr_data(0x07);
    lcd_wr_data(0x0A);
    lcd_wr_data(0x28);
    lcd_wr_data(0x32);
    lcd_wr_data(0x44);
    lcd_wr_data(0x42);
    lcd_wr_data(0x06);
    lcd_wr_data(0x0E);
    lcd_wr_data(0x12);
    lcd_wr_data(0x14);
    lcd_wr_data(0x17);
    lcd_wr_regno(0xE1);
    lcd_wr_data(0xd0);
    lcd_wr_data(0x00);
    lcd_wr_data(0x02);
    lcd_wr_data(0x07);
    lcd_wr_data(0x0A);
    lcd_wr_data(0x28);
    lcd_wr_data(0x31);
    lcd_wr_data(0x54);
    lcd_wr_data(0x47);
    lcd_wr_data(0x0E);
    lcd_wr_data(0x1C);
    lcd_wr_data(0x17);
    lcd_wr_data(0x1B);
    lcd_wr_data(0x1E);
    lcd_wr_regno(0x2A);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0xEF);
    lcd_wr_regno(0x2B);
    lcd_wr_data(0x00);
    lcd_wr_data(0x00);
    lcd_wr_data(0x01);
    lcd_wr_data(0x3F);
    lcd_wr_regno(0x29);
}


/**
 * @brief   初始化LCD
 * @param   无
 * @retval  无
 */
void lcd_init(void)
{
    //引脚定义句柄
    GPIO_InitTypeDef gpio_init_struct;
    
    //静态存储控制器数据读取句柄
    FSMC_NORSRAM_TimingTypeDef fsmc_read_timing_struct = {0};
    
    //静态存储控制器数据写入句柄
    FSMC_NORSRAM_TimingTypeDef fsmc_write_timing_struct = {0};
    
    /* 使能B类型引脚时钟 */
    LCD_BL_GPIO_CLK_ENABLE();
    
    //设置LCD BL背光引脚信息
    gpio_init_struct.Pin = LCD_BL_GPIO_PIN;
    
    //设置背光引脚模式为推挽输出
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    
    //设置背光引脚为上拉
    gpio_init_struct.Pull = GPIO_PULLUP;
    
    //设置背光引脚速度为高速
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    
    //根据背光引脚参数进行初始化
    HAL_GPIO_Init(LCD_BL_GPIO_PORT, &gpio_init_struct);
    
    
    
    // 配置FSMC读时序
    //配置地址建立时间所需的HCLK周期数
    fsmc_read_timing_struct.AddressSetupTime = 15;
    
    //配置地址保持时间所需的HCLK周期数
    fsmc_read_timing_struct.AddressHoldTime = 0;
    
    //配置数据建立时间所需的HCLK周期数
    fsmc_read_timing_struct.DataSetupTime = 60;
    
    //选择异步访问模式A
    fsmc_read_timing_struct.AccessMode = FSMC_ACCESS_MODE_A;
    
    
    
    //配置FSMC写时序
    //配置地址建立时间所需的HCLK周期数
    fsmc_write_timing_struct.AddressSetupTime = 9;
    
    //配置地址保持时间所需的HCLK周期数
    fsmc_write_timing_struct.AddressHoldTime = 0;
    
    //配置数据建立时间所需的HCLK周期数
    fsmc_write_timing_struct.DataSetupTime = 9;
    
    //选择异步访问模式A
    fsmc_write_timing_struct.AccessMode = FSMC_ACCESS_MODE_A;
 
    //配置FSMC
    //使用FSMC 的存储块1（Bank1，专门用于 NOR Flash / SRAM / LCD）
    g_sram_handle.Instance = FSMC_NORSRAM_DEVICE;
    
    //指定使用存储块1 的“扩展模式”配置区（用于设置额外的时序参数，如读/写不同时序）
    g_sram_handle.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
    
    // ★ 核心：选择 存储块1 下的第几个子 区域块（片选引脚） 
    // 这里选的是 NE4（BANK4），对应开发板上的 FSMC_NE4 引脚（通常接 LCD 的 CS）
    g_sram_handle.Init.NSBank = FSMC_NORSRAM_BANK4;
    
    // 地址/数据线是否复用（即数据线和地址线是否共用同一组引脚）
    // DISABLE = 不复用，即地址线（A0~Axx）和数据线（D0~D15）是分开的（LCD 通常是这样）
    g_sram_handle.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
    
    // 外部设备的存储器类型：这里是 SRAM（LCD 的 8080 接口时序和 SRAM 很相似）
    // 也可以配置为 NOR Flash 或 PSRAM，但驱动 LCD 通常设为 SRAM
    g_sram_handle.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
    
    // 数据总线宽度：16 位（即数据线 D0~D15 全部使用）
    // LCD 通常是 16 位颜色深度（RGB565），所以用 16 位
    g_sram_handle.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
    
    // 是否启用突发访问模式（Burst Mode）：DISABLE 表示不使用
    // 突发模式适合连续快速读取，但 LCD 不需要，关闭更稳定
    g_sram_handle.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
    
    // 等待信号的极性（如果使用了等待信号，这里配置低电平有效）
    // 因为这里没用等待信号，所以设成 LOW 没什么影响
    g_sram_handle.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
    
    // 突发模式下是否启用“回绕模式”（Wrap Mode）：DISABLE
    // 与突发模式配合使用，这里关闭
    g_sram_handle.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
    
    // 等待信号是在“等待状态之前”还是“等待状态期间”有效：这里设在前
    // 具体细节不用深究，标准配置即可
    g_sram_handle.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
    
    // 是否允许写操作：ENABLE（当然要允许，否则没法往屏幕发数据）
    g_sram_handle.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
    
    //是否使用等待信号引脚：DISABLE（LCD 不需要等待信号，按最快速度传）
    g_sram_handle.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
    
    // 是否启用扩展模式（Extended Mode）：ENABLE
    // 启用后可以分别配置读时序和写时序（不同速度），提高灵活性
    g_sram_handle.Init.ExtendedMode = FSMC_EXTENDED_MODE_ENABLE;
    
    // 是否启用异步等待：DISABLE（只用同步方式，速度更快）
    g_sram_handle.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
    
    // 是否启用“写突发”模式：DISABLE（普通写操作，不用突发）
    g_sram_handle.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
    
    // 页面大小（用于 NAND Flash 等），这里是 NOR/SRAM，所以设为 NONE（无页面）
    g_sram_handle.Init.PageSize = FSMC_PAGE_SIZE_NONE;
    
    //根据参数初始化SRAM
    HAL_SRAM_Init(&g_sram_handle, &fsmc_read_timing_struct, &fsmc_write_timing_struct);
    delay_ms(50);
    
    //获取设备ID（对于支持多设备才需要这样做，当前只是一个设备（0x7789），可省略）
    lcd_wr_regno(0x04);
    
    //dummy read
    lcddev.id = lcd_rd_data();
    
    //读到0x85
    lcddev.id = lcd_rd_data();
    
    //读取0x85
    lcddev.id = lcd_rd_data();
    
    //左移八位
    lcddev.id <<= 8;
    
    //读取0x52
    lcddev.id |= lcd_rd_data();

    //将8552的ID转换成7789
    if (lcddev.id == 0x8552)
        {
            lcddev.id = 0x7789;
       }
    
        //执行ST7789初始化 
        lcd_ex_st7789_reginit();
    
        //初始化完成后，提速 ,将地址建立时间从所需的9个HCLK周期改为3个HCLK周期
        fsmc_write_timing_struct.AddressSetupTime = 3;
        
        //初始化完成后，提速 ,将数据建立时间从所需的9个HCLK周期改为3个HCLK周期
        fsmc_write_timing_struct.DataSetupTime = 3;
       
        //根据参数初始化FSMC_NORSRAM扩展模式的时序，为 LCD 屏的“写入数据”行为，加载一套专属的、更合适的快速时间参数
        FSMC_NORSRAM_Extended_Timing_Init(g_sram_handle.Extended, &fsmc_write_timing_struct, g_sram_handle.Init.NSBank, g_sram_handle.Init.ExtendedMode);
    
    
        //默认设置为竖屏
        lcd_display_dir(0);
       
        //点亮背光
        LCD_BL(1);
       
        //清屏,为白色
        lcd_clear(WHITE);
}





/**
 * @brief   HAL库SRAM初始化MSP函数（由 HAL_SRAM_Init 自动调用）
 * @param   hsram: SRAM句柄指针，这里传进来的是 &g_sram_handle
 * @retval  无
 */
void HAL_SRAM_MspInit(SRAM_HandleTypeDef *hsram)
{
    // 定义一个GPIO初始化结构体，用来配置引脚
    GPIO_InitTypeDef gpio_init_struct;   
    
    // 判断是不是我们用的那个 FSMC 设备（防止多个SRAM设备）
    if (hsram->Instance == FSMC_NORSRAM_DEVICE)
    {
        /* -------- 1. 使能所有需要用到的外设时钟 -------- */
        // 开启 FSMC 外设的时钟（必须）
        __HAL_RCC_FSMC_CLK_ENABLE();

        // 开启 GPIOD 时钟（数据线 D0~D15 等）    
        __HAL_RCC_GPIOD_CLK_ENABLE();

        //开启 GPIOE 时钟（部分数据线）
        __HAL_RCC_GPIOE_CLK_ENABLE();
        
        // 下面这几个宏是 lcd.h 里自定义的，用来开启 WR/RD/CS/RS 所在的 GPIO 时钟
        // 开启 WR 引脚（GPIOD5）的时钟
        LCD_WR_GPIO_CLK_ENABLE();

        // 开启 RD 引脚（GPIOD4）的时钟
        LCD_RD_GPIO_CLK_ENABLE();

        // 开启 CS 引脚（GPIOG12）的时钟
        LCD_CS_GPIO_CLK_ENABLE();

        // 开启 RS 引脚（GPIOF12）的时钟
        LCD_RS_GPIO_CLK_ENABLE();         
        
        /* -------- 2. 配置 FSMC 的所有数据/地址/控制引脚为复用推挽输出 -------- */
        
        // 配置 GPIOD 的部分引脚：D0, D1, D8, D9, D10, D14, D15（作为数据/地址线）
        gpio_init_struct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
        
        // 复用推挽输出模式
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;

        // 内部上拉，保证空闲电平稳定
        gpio_init_struct.Pull = GPIO_PULLUP;

        // 高速（用于 FSMC 快速翻转）
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;

        // 复用功能编号 12（FSMC）
        gpio_init_struct.Alternate = GPIO_AF12_FSMC;

        // 根据参数初始化引脚
        HAL_GPIO_Init(GPIOD, &gpio_init_struct);          
        
        // 配置 GPIOE 的部分引脚：E7~E15（也是数据/地址线）
        gpio_init_struct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
        
        // 注意：上面的 Alternat 和 Speed 等复用前面设置，这里直接调用
        HAL_GPIO_Init(GPIOE, &gpio_init_struct);          
        
        
        
        // 配置 WR写引脚，PD5
        gpio_init_struct.Pin = LCD_WR_GPIO_PIN;
        
        //配置引脚复用号
        gpio_init_struct.Alternate = LCD_WR_GPIO_AF;
        
        //根据参数初始化对应WR引脚
        HAL_GPIO_Init(LCD_WR_GPIO_PORT, &gpio_init_struct);
        
        
        
        // 配置 RD读引脚，PD4
        gpio_init_struct.Pin = LCD_RD_GPIO_PIN;

        //配置引脚复用号
        gpio_init_struct.Alternate = LCD_RD_GPIO_AF;
        
        //根据参数初始化对应RD引脚
        HAL_GPIO_Init(LCD_RD_GPIO_PORT, &gpio_init_struct);
        
        
        
        // 配置 CS（片选）引脚,PG12
        gpio_init_struct.Pin = LCD_CS_GPIO_PIN;
        
        //配置引脚复用号
        gpio_init_struct.Alternate = LCD_CS_GPIO_AF;
        
        //根据参数初始化对应CS引脚
        HAL_GPIO_Init(LCD_CS_GPIO_PORT, &gpio_init_struct);
        
        
        
        // 配置 RS（命令/数据选择）引脚，PF12
        gpio_init_struct.Pin = LCD_RS_GPIO_PIN;
        
        //配置引脚复用号
        gpio_init_struct.Alternate = LCD_RS_GPIO_AF;

        //根据参数初始化对应RS引脚
        HAL_GPIO_Init(LCD_RS_GPIO_PORT, &gpio_init_struct);
    }
}


/**
 * @brief   在指定位置显示一个中文字符（顺序索引模式）
 * @param   x: 起始X坐标
 * * @param   y: 起始Y坐标
 * @param   str: 中文字符串（字库顺序必须与字符串一致）
 * @param   size: 字体大小（16/24/32）
 * @param   color: 显示颜色（RGB565）
 * @retval  无
 * @note    字库顺序必须与字符串中的字符顺序完全对应
 */
void lcd_show_chinese(uint16_t x, uint16_t y, char *str, uint8_t size, uint16_t color)
{
    uint8_t j, k, tmp;
    uint16_t num;
    uint8_t char_index = 0;
    uint16_t x0 = x;
   
    
    while (*str) {
        if (size == 16) {
            num = sizeof(cfont16) / sizeof(typFNT_GB16);
            if (char_index < num) {
                // 16x16 点阵，每字 32 字节，每行 2 字节（16位），共 16 行
                for (j = 0; j < 32; j++) {
                    tmp = cfont16[char_index].Msk[j];
                    // 一字节对应 8 个像素点
                    for (k = 0; k < 8; k++) {
                        if (tmp & (0x80 >> k)) {
                            // 画点
                            lcd_draw_point(x + (j % 2) * 8 + k, y + j / 2, color);
                        }
                        // 注意：这里没有画背景，因为 TFT 通常不需要背景色，或者使用清屏颜色
                        // 如果希望背景色，可以在 else 中画背景色，但会影响速度
                    }
                }
            }
            x += size; // 下一个汉字横向偏移 16 像素
            char_index++;
        } else if (size == 24) {
            num = sizeof(cfont24) / sizeof(typFNT_GB24);
            if (char_index < num) {
                // 24x24 点阵，每字 72 字节，每行 3 字节（24位），共 24 行
                for (j = 0; j < 72; j++) {
                    tmp = cfont24[char_index].Msk[j];
                    for (k = 0; k < 8; k++) {
                        if (tmp & (0x80 >> k)) {
                            lcd_draw_point(x + (j % 3) * 8 + k, y + j / 3, color);
                        }
                    }
                }
            }
            x += size;
            char_index++;
        } else if (size == 32) {
            num = sizeof(cfont32) / sizeof(typFNT_GB32);
            if (char_index < num) {
                // 32x32 点阵，每字 128 字节，每行 4 字节（32位），共 32 行
                for (j = 0; j < 128; j++) {
                    tmp = cfont32[char_index].Msk[j];
                    for (k = 0; k < 8; k++) {
                        if (tmp & (0x80 >> k)) {
                            lcd_draw_point(x + (j % 4) * 8 + k, y + j / 4, color);
                        }
                    }
                }
            }
            x += size;
            char_index++;
        }
        
        str += 2;  // 跳过两个字节（GBK编码）
        // 如果超出水平边界，换行（简单处理）
        if (x > lcddev.width - size) {
            x = x0;
            y += size;
        }
        // 如果超出垂直边界，停止显示
        if (y > lcddev.height - size) {
            break;
        }
    }
}

