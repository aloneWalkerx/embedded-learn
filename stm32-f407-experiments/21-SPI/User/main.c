#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/KEY/key-i.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/NORFLASH/norflash.h"

/* ====================================================================
   全局变量定义
   ==================================================================== */

// ★ 要写入 Flash 的测试数据（字符串末尾自动包含 '\0'）
// 写入内容："STM32 SPI TEST"
// 共 15 个字符（"STM32 SPI TEST" = 14 个字符 + 1 个结束符）
static const uint8_t g_text_buf[] = {"STM32 SPI TEST"};

// ★ 计算测试数据的长度（包含结束符 '\0'）
// TEXT_SIZE = sizeof(g_text_buf) = 15 字节
#define TEXT_SIZE sizeof(g_text_buf)

/* ====================================================================
   主函数
   ==================================================================== */
int main(void)
{
    /* --- 局部变量定义 --- */
    uint8_t t = 0;          // 用于 LED0 闪烁计数（每 20 次翻转一次）
    uint8_t key;            // 存储按键扫描结果
    uint8_t data[TEXT_SIZE]; // 读取数据的缓冲区（与写入数据大小相同）
    uint16_t id;            // 存储 NOR Flash 芯片 ID
    uint32_t flashsize;     // NOR Flash 容量（字节）
    
    /* --- 系统初始化（按照依赖关系逐一初始化） --- */
    HAL_Init();                         // 初始化 HAL 库（必须最先调用）
    sys_stm32_clock_init(336, 8, 2, 7); // 配置系统时钟为 168MHz
    delay_init(168);                    // 初始化延时函数（基于 SysTick）
    usart_init(115200);                 // 初始化串口 1（波特率 115200）
    led_i_init();                       // 初始化 LED（GPIO 配置）
    key_i_init();                       // 初始化按键（GPIO 配置）
    lcd_init();                         // 初始化 LCD（FSMC + ST7789 驱动）
    norflash_init();                    // ★ 初始化 NOR Flash（SPI1 + W25Q128）
    
    /* --- LCD 显示固定标题信息 --- */
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);           // 第 1 行：开发板型号
    lcd_show_string(30, 70, 200, 16, 16, "SPI TEST", RED);        // 第 2 行：实验名称
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);   // 第 3 行：作者/版权信息
    lcd_show_string(30, 110, 200, 16, 16, "WK_UP:Write KEY0:Read", RED); // 第 4 行：操作提示
    
    /* --- ★ 检测 NOR Flash 是否正常 --- */
    id = norflash_read_id();    // 读取芯片 ID（W25Q128 返回 0xEF17）
    
    // 如果读取到的 ID 为 0 或 0xFFFF，说明芯片未连接或异常
    while ((id == 0) || (id == 0xFFFF))
    {
        lcd_show_string(30, 130, 200, 16, 16, "NOR Flash Check Failed!", RED);
        delay_ms(500);
        lcd_show_string(30, 130, 200, 16, 16, "Please Check!          ", RED);
        delay_ms(500);
        LED0_TOGGLE();  // LED 闪烁提示异常
        id = norflash_read_id(); // 重新读取 ID
    }
    
    // ★ 芯片检测通过，显示就绪信息
    lcd_show_string(30, 130, 200, 16, 16, "NOR Flash Ready!", RED);
    
    // ★ 设置 Flash 容量为 16MB（W25Q128 = 128Mbit = 16MByte）
    // 为什么写入地址选在 16MB - 15 字节？
    // 因为要写入末尾区域，避免破坏前面可能存在的有效数据
    // 这样即使之前的实验数据被覆盖，也不影响当前实验验证
    flashsize = 16 * 1024 * 1024;   // 16MB
    
    /* --- ★ 主循环（程序的核心逻辑） --- */
    while (1)
    {
        /* ① 检测按键输入（非阻塞扫描） */
        key = key_i_scan(0);    // 0 表示不等待按键释放（非阻塞）
        
        /* ② ★ 按下 WKUP 键 → 写入数据到 Flash */
        if (key == WKUP_PRESS)   // WKUP 按下
        {
            /* 清空 LCD 下半屏（Y 坐标 150~319 区域） */
            lcd_fill(0, 150, 239, 319, WHITE);
            
            /* 显示开始写入提示 */
            lcd_show_string(30, 150, 200, 16, 16, "Start Write Flash....", BLUE);
            
            /* ★ 核心操作：向 Flash 写入数据
             * 参数 1：要写入的数据缓冲区（g_text_buf）
             * 参数 2：写入起始地址（Flash 末尾区域：16MB - 15 字节）
             * 参数 3：写入数据长度（15 字节）
             * 
             * ★ 为什么选择末尾地址？
             * - 避免破坏 Flash 前面区域可能已有的数据
             * - 读取时也使用同样的地址，确保读到的是刚写入的数据
             * - 便于实验验证
             */
            norflash_write((uint8_t *)g_text_buf, flashsize - TEXT_SIZE, TEXT_SIZE);
            
            /* 显示写入完成提示 */
            lcd_show_string(30, 150, 200, 16, 16, "Flash Write Finished!", BLUE);
        }
        /* ③ ★ 按下 KEY0 键 → 从 Flash 读取数据 */
        else if (key == KEY0_PRESS)   // KEY0 按下
        {
            /* 显示开始读取提示 */
            lcd_show_string(30, 150, 200, 16, 16, "Start Read Flash.... ", BLUE);
            
            /* ★ 核心操作：从 Flash 读取数据
             * 参数 1：存放读取数据的缓冲区（data）
             * 参数 2：读取起始地址（与写入地址一致）
             * 参数 3：读取数据长度（15 字节）
             */
            norflash_read(data, flashsize - TEXT_SIZE, TEXT_SIZE);
            
            /* 显示读取完成提示和读取到的数据 */
            lcd_show_string(30, 150, 200, 16, 16, "The Data Readed Is:  ", BLUE);
            lcd_show_string(30, 170, 200, 16, 16, (char *)data, BLUE);
        }
        
        /* ④ LED0 闪烁（指示程序正常运行） */
        if (++t == 20)      // t 从 0 到 19，每 10ms 加 1，20 次 = 200ms
        {
            t = 0;          // 重置计数器
            LED0_TOGGLE();  // 翻转 LED0（亮/灭交替）
        }
        
        /* ⑤ 主循环延时 10ms（控制 LED 闪烁频率和按键扫描周期） */
        delay_ms(10);       // 每 10ms 循环一次
    }
}
