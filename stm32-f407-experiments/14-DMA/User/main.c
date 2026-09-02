#include "./SYSTEM/sys/sys.h"        
#include "./SYSTEM/delay/delay.h"    
#include "./SYSTEM/usart/usart.h"    
#include "./BSP/LED/led-i.h"         
#include "./BSP/KEY/key-i.h"         
#include "./BSP/LCD/lcd.h"           
#include "./BSP/DMA/dma.h"           

/* ====================================================================
   外部变量声明
   ==================================================================== */
// ★ 声明外部变量 g_uart1_handle（在 usart.c 中定义）
// 这个句柄包含了 USART1 的所有配置信息，HAL 库需要它来操作串口
// 我们在 dma.c 中已经把 DMA 句柄绑定到了这个 UART 句柄上
extern UART_HandleTypeDef g_uart1_handle;

/* ====================================================================
   全局变量定义
   ==================================================================== */
// ★ 原始数据模板：要发送的字符串（带换行符）
// sizeof(temp) 会自动计算数组大小（包含末尾的 '\0'）
// 但 DMA 发送时只发送有效数据，不包括 '\0'（因为 HAL_UART_Transmit_DMA 的参数是长度）
uint8_t temp[] = {"Hello, M144Z-M4 STM32F407ZGT6!\r\n"};

// ★ 大容量发送缓冲区：将 temp 重复 200 次，组成一个大的数据包
// sizeof(temp) * 200 计算总字节数，用于模拟大量数据传输
// 这样做的目的是展示 DMA 搬运大数据块的能力
uint8_t buf[sizeof(temp) * 200];

// ★ 串口发送就绪标志（1=可以发送，0=正在发送中）
// 这是一个关键的状态机标志，用于防止在上一次 DMA 传输未完成时再次启动
uint8_t uart_ready = 1;

/* ====================================================================
   ① HAL 库 UART 传输完成回调函数
   ==================================================================== */
/**
 * @brief   HAL库UART传输完成回调函数
 * @param   huart: UART 句柄指针
 * @retval  无
 * @note    此函数由 HAL 库在 DMA 传输完成中断中自动调用
 *          我们在这里标记传输完成，让主循环可以发起下一次传输
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    /* ★ 标记传输完成，可以进行下一次传输 */
    // 当 DMA 把 buf 中的所有数据都发送到串口后，硬件触发中断
    // 中断服务函数调用此回调，我们在这里把标志置 1
    // 主循环检测到 uart_ready == 1 后，可以再次按下 KEY0 发起新一轮传输
    uart_ready = 1;
}

/* ====================================================================
   ② 主函数
   ==================================================================== */
int main(void)
{
    /* --- 局部变量定义 --- */
    uint8_t t = 0;          // 用于 LED0 闪烁计数（每 20 次翻转一次）
    uint8_t key;            // 存储按键扫描结果
    uint16_t buf_index;     // 大缓冲区索引（0~199）
    uint8_t temp_index;     // 模板缓冲区索引（0~sizeof(temp)-1）
    
    /* --- 系统初始化（按照顺序逐一初始化） --- */
    HAL_Init();                         // 初始化 HAL 库（必须最先调用）
    sys_stm32_clock_init(336, 8, 2, 7); // 配置系统时钟为 168MHz
    delay_init(168);                    // 初始化延时函数（基于 SysTick）
    usart_init(115200);                 // 初始化串口 1（波特率 115200）
    led_i_init();                       // 初始化 LED（GPIO 配置）
    key_i_init();                       // 初始化按键（GPIO 配置，用于触发 DMA 发送）
    lcd_init();                         // 初始化 LCD（显示提示信息）
    dma_init();                         // ★ 初始化 DMA（配置 DMA2_Stream7 用于串口发送）
    
    /* --- LCD 显示固定信息 --- */
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "DMA TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ALONE@WALKER", RED);
    lcd_show_string(30, 110, 200, 16, 16, "KEY0:Start", RED);
    // 提示用户：按下 KEY0 启动 DMA 发送
    
    /* --- ★ 准备要发送的数据（在内存中构建一个大数据包） --- */
    // 目的：将 temp 字符串重复 200 次，填充到 buf 数组中
    // 这样就能模拟发送大量数据（约 200 × 35 = 7000 字节）
    // 在真实场景中，这些数据可能来自传感器、文件、图像等
    for (buf_index = 0; buf_index < 200; buf_index++)      // 外层循环：重复 200 次
    {
        for (temp_index = 0; temp_index < sizeof(temp); temp_index++)  // 内层循环：复制一个 temp
        {
            // ★ 将 temp 中的每个字节复制到 buf 的对应位置
            // buf_index * sizeof(temp) 计算当前批次的起始偏移
            // 例如：第 0 次复制到 buf[0~34]，第 1 次复制到 buf[35~69]，...
            buf[buf_index * sizeof(temp) + temp_index] = temp[temp_index];
        }
    }
    // 执行完这段代码后，buf 中存放的就是 temp 重复 200 次的数据
    // 总长度 = sizeof(temp) * 200（约 7000 字节）
    
    /* --- ★ 主循环（程序的核心逻辑） --- */
    while (1)
    {
        /* ① 检测按键输入（非阻塞扫描） */
        key = key_i_scan(0);    // 0 表示不等待按键释放（非阻塞）
        
        /* ② 如果 KEY0 被按下，且串口空闲 */
        if (key == KEY0_PRESS)   // KEY0 按下（低电平有效）
        {
            if (uart_ready == 1) // 检查是否允许发送（上一次发送已完成）
            {
                // ★ 启动 DMA 传输
                // 将 uart_ready 置 0，表示正在发送中，阻止重复触发
                uart_ready = 0;
                
                /* ★ 启动 DMA 发送（核心操作！）
                 * 参数 1：UART 句柄（g_uart1_handle）
                 * 参数 2：要发送的数据缓冲区（buf）
                 * 参数 3：要发送的字节数（sizeof(buf)）
                 * 
                 * ★ 执行这条语句后，DMA 控制器会自动完成以下工作：
                 *   1. 从 buf[0] 读取第一个字节
                 *   2. 写入 USART1 的发送数据寄存器（USART_DR）
                 *   3. 等待发送完成标志（TXE）
                 *   4. 自动从 buf[1] 读取下一个字节
                 *   5. 重复上述步骤，直到发送完 sizeof(buf) 个字节
                 *   6. 触发传输完成中断 → 调用 HAL_UART_TxCpltCallback
                 * 
                 * ★ 在 DMA 发送期间，CPU 可以执行其他任务！
                 *    这就是 DMA 的核心价值：释放 CPU
                 */
                HAL_UART_Transmit_DMA(&g_uart1_handle, buf, sizeof(buf));
            }
            // 如果 uart_ready == 0，说明上次发送还没完成，忽略此次按键
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