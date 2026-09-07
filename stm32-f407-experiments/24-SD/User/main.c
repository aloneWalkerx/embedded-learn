#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./MALLOC/malloc.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/KEY/key-i.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/SD/sdcard.h"

/* ====================================================================
   ① 显示 SD 卡信息
   ==================================================================== */
/**
 * @brief   显示SD卡信息
 * @param   无
 * @retval  无
 * @note    打印 SD 卡的 CID（Card IDentification）信息到串口
 *          包括：卡类型、制造商ID、RCA、容量、块大小等
 *          同时在 LCD 上显示容量
 */
static void show_sd_info(void)
{
    HAL_SD_CardCIDTypeDef sd_card_cid = {0};   // CID 结构体（存储卡标识信息）
    
    // ★ 获取 SD 卡的 CID 信息（包括制造商ID、产品名称、序列号等）
    HAL_SD_GetCardCID(&g_sd_handle, &sd_card_cid);
    
    /* --- 打印卡类型 --- */
    // CARD_SDSC：标准容量卡（≤2GB）
    // CARD_SDHC_SDXC：高容量卡（≥4GB）
    // CARD_SECURED：安全卡
    printf("Card Type: %s\r\n", (g_sd_card_info.CardType == CARD_SDSC) ? ((g_sd_card_info.CardVersion == CARD_V1_X) ? ("SDSC V1") :
                                                                         ((g_sd_card_info.CardVersion == CARD_V1_X) ? ("SDSC V2") :
                                                                         (""))) :
                                ((g_sd_card_info.CardType == CARD_SDHC_SDXC) ? ("SDHC") :
                                ((g_sd_card_info.CardType == CARD_SECURED) ? ("SECURE") :
                                (""))));
    
    // ★ 制造商 ID（由 SD 卡协会分配，不同厂商有不同 ID）
    printf("Card ManufacturerID:%d\r\n", sd_card_cid.ManufacturerID);
    
    // ★ RCA（Relative Card Address）：相对卡地址，由主机分配
    printf("Card RCA:%d\r\n", g_sd_card_info.RelCardAdd);
    
    // ★ 逻辑块数量（总块数）
    printf("LogBlockNbr:%d \r\n", g_sd_card_info.LogBlockNbr);
    
    // ★ 逻辑块大小（通常为 512 字节）
    printf("LogBlockSize:%d \r\n", g_sd_card_info.LogBlockSize);
    
    // ★ 计算并打印 SD 卡容量（单位：MB）
    // 容量 = 逻辑块数量 × 逻辑块大小，>> 20 相当于除以 1048576（1MB）
    printf("Card Capacity:%d MB\r\n", (uint32_t)(((uint64_t)g_sd_card_info.LogBlockNbr * g_sd_card_info.LogBlockSize) >> 20));
    
    // ★ 块大小（通常与逻辑块大小相同）
    printf("Card BlockSize:%d\r\n\r\n", g_sd_card_info.BlockSize);
    
    /* --- 在 LCD 上显示容量 --- */
    lcd_show_string(30, 146, 200, 16, 16, "SD Card Size:     MB", BLUE);
    lcd_show_num(30 + 13 * 8, 146, (uint32_t)(((uint64_t)g_sd_card_info.LogBlockNbr * g_sd_card_info.LogBlockSize) >> 20), 5, 16, BLUE);
}

/* ====================================================================
   ② SD 卡读取测试
   ==================================================================== */
/**
 * @brief   SD 卡读测试
 * @param   无
 * @retval  无
 * @note    ① 从内存池申请一块缓冲区（大小为 1 个块）
 *          ② 读取 SD 卡的第 0 个块（MBR / 引导扇区）
 *          ③ 通过串口打印该块的数据（十六进制）
 *          ④ 释放缓冲区内存
 */
static void sd_read_test(void)
{
    uint8_t *buf;           // 数据缓冲区指针
    uint16_t index;         // 循环索引变量
    
    /* --- 步骤 1：从 SRAM 内存池申请一块缓冲区 --- */
    // ★ mymalloc 是自定义的内存分配函数
    // 分配大小为 1 个块（BlockSize，通常为 512 字节）
    buf = (uint8_t *)mymalloc(SRAMIN, g_sd_card_info.BlockSize);
    if (buf == NULL)        // 内存分配失败
    {
        return;             // 直接返回
    }
    
    /* --- 步骤 2：读取 SD 卡的第 0 个块 --- */
    // sd_read_disk 参数：缓冲区、起始扇区、扇区数量
    // 返回 0 表示成功，非 0 表示失败
    // ★ 第 0 个块通常是 MBR（主引导记录），包含分区表信息
    if (sd_read_disk(buf, 0, 1) == 0)
    {
        /* 读取成功 → 通过串口打印数据 */
        lcd_show_string(30, 170, 200, 16, 16, "USART1 Sending Data...", BLUE);
        printf("Block 0 Data:\r\n");
        
        // ★ 循环打印每个字节（十六进制格式）
        for (index = 0; index < g_sd_card_info.BlockSize; index++)
        {
            printf("%02X ", buf[index]);
        }
        printf("\r\nData End\r\n");
        lcd_show_string(30, 170, 200, 16, 16, "USART1 Send Data Over!", BLUE);
    }
    else                    /* 读取失败 */
    {
        printf("SD read Failure!\r\n");
        lcd_show_string(30, 170, 200, 16, 16, "SD read Failure!      ", BLUE);
    }
    
    /* --- 步骤 3：释放缓冲区内存 --- */
    // ★ myfree 释放之前申请的内存（防止内存泄漏）
    myfree(SRAMIN, buf);
}

/* ====================================================================
   ③ 主函数
   ==================================================================== */
int main(void)
{
    uint8_t t = 0;          // 用于 LED0 闪烁计数（每 20 次翻转一次）
    uint8_t key;            // 存储按键扫描结果
    
    /* --- 系统初始化（按照依赖关系逐一初始化） --- */
    HAL_Init();                         // 初始化 HAL 库（必须最先调用）
    sys_stm32_clock_init(336, 8, 2, 7); // 配置系统时钟为 168MHz
    delay_init(168);                    // 初始化延时函数（基于 SysTick）
    usart_init(115200);                 // 初始化串口 1（波特率 115200）
    led_i_init();                       // 初始化 LED（GPIO 配置）
    key_i_init();                       // 初始化按键（GPIO 配置）
    lcd_init();                         // 初始化 LCD（FSMC + ST7789 驱动）
    
    /* --- ★ 初始化内存池 --- */
    // ★ 自定义内存管理：初始化两个内存池
    // SRAMIN：内部 SRAM（通常用于普通数据）
    // SRAMCCM：CCM 内存（内核耦合内存，速度更快，但不能被 DMA 访问）
    my_mem_init(SRAMIN);                // 初始化内部 SRAM 内存池
    my_mem_init(SRAMCCM);               // 初始化 CCM 内存池
    
    /* --- LCD 显示固定标题信息 --- */
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);           // 第 1 行：开发板型号
    lcd_show_string(30, 70, 200, 16, 16, "SD TEST", RED);         // 第 2 行：实验名称
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);   // 第 3 行：作者/版权信息
    lcd_show_string(30, 110, 200, 16, 16, "WKUP: Read Block 0", RED); // 第 4 行：操作提示

    /* --- ★ 初始化 SD 卡（循环检测直到成功） --- */
    while (sd_init() != 0)   // sd_init 返回 0 表示成功，非 0 表示失败
    {
        /* SD 卡初始化失败 → 显示错误提示并等待 */
        lcd_show_string(30, 130, 200, 16, 16, "SD Card Error!", RED);
        delay_ms(500);
        lcd_show_string(30, 130, 200, 16, 16, "Please Check! ", RED);
        delay_ms(500);
        LED0_TOGGLE();       // LED 闪烁提示错误
        // ★ 循环直到 SD 卡插入或初始化成功
    }
    
    /* SD 卡初始化成功 → 显示成功信息 */
    lcd_show_string(30, 130, 200, 16, 16, "SD Card OK    ", BLUE);
    
    /* --- ★ 显示 SD 卡信息 --- */
    show_sd_info();          // 打印和显示 SD 卡容量、类型等信息

    /* --- ★ 主循环 --- */
    while (1)
    {
        /* ① 检测按键输入（非阻塞扫描） */
        key = key_i_scan(0);    // 0 表示不等待按键释放（非阻塞）
        
        /* ② ★ 按下 WKUP 键 → 读取 SD 卡第 0 块数据 */
        if (key == WKUP_PRESS)   // WKUP 按下
        {
            /* 进行 SD 卡读测试（读取第 0 块并通过串口打印） */
            sd_read_test();
        }
        
        /* ③ LED0 闪烁（指示程序正常运行） */
        if (++t == 20)      // t 从 0 到 19，每 10ms 加 1，20 次 = 200ms
        {
            t = 0;          // 重置计数器
            LED0_TOGGLE();  // 翻转 LED0（亮/灭交替）
        }
        
        /* ④ 主循环延时 10ms（控制 LED 闪烁频率和按键扫描周期） */
        delay_ms(10);       // 每 10ms 循环一次
    }
}
