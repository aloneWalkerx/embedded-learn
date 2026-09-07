#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/KEY/key-i.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/FLASH/flash.h"

/* ====================================================================
   全局变量定义
   ==================================================================== */

// ★ 要写入 Flash 的测试数据（字符串末尾自动包含 '\0'）
// 写入内容："STM32 FLASH TEST"（共 17 个字符 + 1 个结束符 = 18 字节）
static const uint8_t g_text_buf[] = {"STM32 FLASH TEST"};

// ★ 计算 TEXT_SIZE（4 字节对齐，用于 32 位写入）
// 内部 Flash 写入要求 4 字节对齐，所以需要将大小向上取整到 4 的倍数
// 计算过程：
//   sizeof(g_text_buf) = 18（字符串 "STM32 FLASH TEST\0"）
//   >> 2：右移 2 位 = 18 / 4 = 4（取整，丢弃余数）
//   << 2：左移 2 位 = 4 * 4 = 16
//   + 4：加上 4 字节 = 20
//   这样 TEXT_SIZE 就是 20，确保是 4 的倍数，满足 Flash 写入对齐要求
#define TEXT_SIZE (((sizeof(g_text_buf) >> 2) << 2) + 4)

/* ====================================================================
   主函数
   ==================================================================== */
int main(void)
{
    /* --- 局部变量定义 --- */
    uint8_t t = 0;          // 用于 LED0 闪烁计数（每 20 次翻转一次）
    uint8_t key;            // 存储按键扫描结果
    uint8_t data[TEXT_SIZE]; // 读取数据的缓冲区（大小与写入一致）
    uint8_t wdata[TEXT_SIZE] = {0}; // 写入数据缓冲区（全部初始化为 0）
    uint8_t index;          // 循环索引变量

    /* --- 系统初始化（按照依赖关系逐一初始化） --- */
    HAL_Init();                         // 初始化 HAL 库（必须最先调用）
    sys_stm32_clock_init(336, 8, 2, 7); // 配置系统时钟为 168MHz
    delay_init(168);                    // 初始化延时函数（基于 SysTick）
    usart_init(115200);                 // 初始化串口 1（波特率 115200）
    led_i_init();                       // 初始化 LED（GPIO 配置）
    key_i_init();                       // 初始化按键（GPIO 配置）
    lcd_init();                         // 初始化 LCD（FSMC + ST7789 驱动）

    /* --- LCD 显示固定标题信息 --- */
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);           // 第 1 行：开发板型号
    lcd_show_string(30, 70, 200, 16, 16, "FLASH TEST", RED);      // 第 2 行：实验名称
    lcd_show_string(30, 90, 200, 16, 16, "Alone@Walker", RED);   // 第 3 行：作者信息
    lcd_show_string(30, 110, 200, 16, 16, "WK_UP:Write KEY0:Read", RED); // 第 4 行：操作提示

    /* --- ★ 准备要写入的数据 --- */
    // 将 g_text_buf 的内容复制到 wdata 中
    // wdata 的大小是 4 字节对齐的（20 字节），g_text_buf 是 18 字节
    // 复制后，wdata 末尾会多出 2 个字节的 0（因为 wdata 初始化为 0）
    for (index = 0; index < sizeof(g_text_buf); index++)
    {
        wdata[index] = g_text_buf[index];   // 复制测试数据到写入缓冲区
    }
    // 此时 wdata 内容： "STM32 FLASH TEST" + 2 个字节的 0x00（填充到 20 字节）

    /* --- ★ 主循环（程序的核心逻辑） --- */
    while (1)
    {
        /* ① 检测按键输入（非阻塞扫描） */
        key = key_i_scan(0);    // 0 表示不等待按键释放（非阻塞）

        /* ② ★ 按下 WKUP 键 → 写入数据到内部 Flash */
        if (key == WKUP_PRESS)   // WKUP 按下
        {
            /* 清空 LCD 下半屏（Y 坐标 130~319 区域） */
            lcd_fill(0, 130, 239, 319, WHITE);

            /* 显示开始写入提示 */
            lcd_show_string(30, 130, 200, 16, 16, "Start Write Flash....", BLUE);

            /* ★ 核心操作：向内部 Flash 写入数据
             * 参数 1：目标地址 = FLASH_END + 1 - TEXT_SIZE
             *        FLASH_END 是 Flash 的末地址（0x080FFFFF）
             *        所以目标地址是 Flash 末尾往前 20 字节的位置
             *        这样数据存放在 Flash 的最末尾，不会影响程序代码
             * 参数 2：要写入的数据缓冲区（wdata，20 字节，4 字节对齐）
             * 参数 3：要写入的 32 位数据个数 = TEXT_SIZE / 4 = 20/4 = 5
             * 
             * ★ 为什么选择末尾地址？
             * - 程序代码从 0x08000000 开始存放
             * - 使用末尾区域存放用户数据，避免擦除/写入时破坏程序代码
             * - 即使写入失败，也不影响程序正常运行
             */
            flash_write(FLASH_END + 1 - TEXT_SIZE, (uint32_t *)wdata, TEXT_SIZE / sizeof(uint32_t));

            /* 显示写入完成提示 */
            lcd_show_string(30, 130, 200, 16, 16, "Flash Write Finished!", BLUE);
        }
        /* ③ ★ 按下 KEY0 键 → 从内部 Flash 读取数据 */
        else if (key == KEY0_PRESS)   // KEY0 按下
        {
            /* 显示开始读取提示 */
            lcd_show_string(30, 130, 200, 16, 16, "Start Read Flash.... ", BLUE);

            /* ★ 核心操作：从内部 Flash 读取数据
             * 参数 1：目标地址（与写入地址一致）
             * 参数 2：存放读取数据的缓冲区（data）
             * 参数 3：要读取的 32 位数据个数
             * 
             * ★ 内部 Flash 读取不需要解锁，直接通过指针读取即可
             * stmflash_read 内部就是将地址强制转换为指针，直接读取
             */
            flash_read(FLASH_END + 1 - TEXT_SIZE, (uint32_t *)data, TEXT_SIZE / sizeof(uint32_t));

            /* 显示读取完成提示和读取到的数据 */
            lcd_show_string(30, 130, 200, 16, 16, "The Data Readed Is:  ", BLUE);
            lcd_show_string(30, 150, 200, 16, 16, (char *)data, BLUE);
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
