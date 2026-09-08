/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-04-23
 * @brief       FATFS实验 - 清理NOR Flash残留文件专用
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./MALLOC/malloc.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/SD/sdcard.h"
#include "./FATFS/exfuns/exfuns.h"
#include "./USMART/usmart.h"
#include "./FATFS/exfuns/fattester.h"   // 包含 mf_scan_files 的声明
#include <string.h>                     // 解决 strcmp 警告

/**
 * @brief       格式化指定的磁盘，并重新挂载
 * @param       drive : 盘符字符串，如 "0:" 或 "1:"
 * @retval      0 成功，其他值失败（错误码）
 */
uint8_t format_drive(char *drive)
{
    FRESULT res;
    uint8_t ret;

    // 1. 先卸载该卷（清空缓存）
    f_mount(NULL, drive, 1);

    // 2. 执行格式化（自动选择FAT类型和簇大小）
    ret = mf_fmkfs((uint8_t *)drive, 0, 0);
    if (ret != 0)
    {
        printf("格式化 %s 失败，错误码: %d\r\n", drive, ret);
        return ret;
    }

    // 3. 重新挂载
    if (drive[0] == '0')
        res = f_mount(fs[0], drive, 1);
    else if (drive[0] == '1')
        res = f_mount(fs[1], drive, 1);
    else
        return 0xFF;

    if (res != FR_OK)
    {
        printf("重新挂载 %s 失败，错误码: %d\r\n", drive, res);
        return res;
    }

    printf("? %s 格式化并重新挂载成功！\r\n", drive);
    return 0;
}

int main(void)
{
    FRESULT res;
    uint32_t total, free;

    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                       /* 初始化LED */
    lcd_init();                         /* 初始化LCD */
    my_mem_init(SRAMIN);                /* 初始化内部SRAM内存池 */
    my_mem_init(SRAMCCM);               /* 初始化CCM内存池 */
    usmart_dev.init(84);                /* 初始化USMART */

    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "FATFS TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "Use USMART for test", RED);
    
    while (sd_init() != 0)
    {
        lcd_show_string(30, 130, 200, 16, 16, "SD Card Error!", RED);
        delay_ms(500);
        lcd_show_string(30, 130, 200, 16, 16, "Please Check! ", RED);
        delay_ms(500);
        LED0_TOGGLE();
    }
    
    exfuns_init();
    
    /* 挂载SD卡 */
    f_mount(fs[0], "0:", 1);
    
    /* 挂载NOR Flash，如果NOR Flash没有文件系统则需要进行格式化 */
    res = f_mount(fs[1], "1:", 1);
    if (res == FR_NO_FILESYSTEM)
    {
        lcd_show_string(30, 130, 200, 16, 16, "Flash Disk Formatting...", RED);
        res = f_mkfs("1:", NULL, NULL, FF_MAX_SS);
        if (res == FR_OK)
        {
            f_setlabel((const TCHAR *)"1:ALIENTEK");
            lcd_show_string(30, 130, 200, 16, 16, "Flash Disk Format Finish", RED);
        }
        else
        {
            lcd_show_string(30, 130, 200, 16, 16, "Flash Disk Format Error ", RED);
        }
        delay_ms(1000);
    }
    lcd_fill(30, 130, 239, 145, WHITE);
    
    /* 获取SD卡容量 */
    while (exfuns_get_free("0", &total, &free) != 0)
    {
        lcd_show_string(30, 130, 200, 16, 16, "SD Card FatFs Error!", RED);
        delay_ms(200);
        lcd_fill(30, 130, 240, 150 + 16, WHITE);
        delay_ms(200);
        LED0_TOGGLE();
    }
    
    lcd_show_string(30, 130, 200, 16, 16, "FATFS OK!", BLUE);
    lcd_show_string(30, 150, 200, 16, 16, "SD Total Size:     MB", BLUE);
    lcd_show_string(30, 170, 200, 16, 16, "SD Free Size:     MB", BLUE);
    lcd_show_num(142, 150, total >> 10, 5, 16, BLUE);
    lcd_show_num(134, 170, free >> 10, 5, 16, BLUE);
    
    while (1)
    {
        delay_ms(200);
        LED0_TOGGLE();
    }
}
