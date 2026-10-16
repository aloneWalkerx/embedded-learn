#include "./SYSTEM/usart/usart.h"                   // 包含 USART 驱动头文件，里面有 USARTX 等宏定义
#include "./SYSTEM/sys/sys.h"

///////////////////////////////////////代码法-不使用半主机模式/////////////////////////////////////////////
#pragma import(__use_no_semihosting)                // 告诉 ARM 链接器不要使用半主机模式，避免程序卡在 BKPT

struct __FILE {                                     // 自定义 FILE 结构体，标准库需要它来支持 fputc
    int handle;                                     // 文件句柄，这里用不到，仅占位

};                                                  // 结构体定义结束

int _ttywrch(int ch)                                // 调试终端输出字符函数，ARM 库会调用
{                                                   // 函数体开始
    ch = ch;                                        // 空操作，消除“未使用参数”警告
    return ch;                                      // 返回传入的字符
}                                                   // 函数结束

void _sys_exit(int x) {                             // 系统退出函数，避免半主机模式下的退出处理
    x = x;                                          // 空操作，消除“未使用参数”警告

}                                                   // 函数结束
char *_sys_command_string(char *cmd, int len) {     // 命令字符串处理函数，半主机相关，这里不需要

    return NULL;                                    // 直接返回空指针
}                                                   // 函数结束
FILE __stdout;                                      // 定义标准输出文件对象，供 fputc 使用

int fputc(int ch, FILE *f) {                        // 重定向 printf 的底层字符输出函数
    while ((USART1->SR &0X40) == 0);                // 等待 USART1 发送数据寄存器为空（TXE 标志位，bit6=0x40）
    USART1->DR = (uint8_t)ch;                       // 把字符写入 USART1 数据寄存器，发送出去
    return ch;                                      // 返回写入的字符

}                                                   // fputc 函数结束
///////////////////////////////////////代码法-不使用半主机模式/////////////////////////////////////////////

UART_HandleTypeDef g_usartx_handle;                 // 定义 UART 句柄，用于 HAL 库操作 USART
uint8_t g_usart_rx_buffer[USARTX_RX_BUFFER_SIZE];   // 定义单字节接收缓冲区，中断接收用
uint8_t g_usartx_rx_buffer_max[USARTX_RX_BUFFER_SIZE_MAX]; // 定义完整帧接收缓冲区，存放一帧数据
uint16_t g_usartx_rx_status_data = 0;               // 定义串口状态/索引变量，低14位存索引，bit14/bit15存标志

void usart_init(uint32_t baudrate) {                // USART 初始化函数，参数为波特率
    g_usartx_handle.Instance = USARTX;              // 选择 USART 外设，例如 USART1
    g_usartx_handle.Init.BaudRate = baudrate;       // 设置波特率
    g_usartx_handle.Init.WordLength = UART_WORDLENGTH_8B; // 设置数据位为 8 位
    g_usartx_handle.Init.StopBits = UART_STOPBITS_1; // 设置停止位为 1 位
    g_usartx_handle.Init.Parity = UART_PARITY_NONE; // 设置无校验
    g_usartx_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE; // 设置无硬件流控
    g_usartx_handle.Init.Mode = UART_MODE_TX_RX;    // 设置模式为收发一体

    HAL_UART_Init(&g_usartx_handle);                // 调用 HAL 库初始化 UART

    HAL_UART_Receive_IT(&g_usartx_handle, g_usart_rx_buffer, USARTX_RX_BUFFER_SIZE); // 开启中断接收，每次收 1 字节

}                                                   // usart_init 函数结束

void HAL_UART_MspInit(UART_HandleTypeDef *uart) {   // HAL 库弱函数，用户实现，用于初始化 MCU 底层外设
    GPIO_InitTypeDef gpio_init_struct;              // 定义 GPIO 初始化结构体
    if (uart->Instance == USARTX) {                 // 判断当前初始化的串口是否是 USARTX
        USARTX_USART1_CLK_ENABLE();                 // 使能 USART1 时钟
        USARTX_TX_GPIO_CLK_ENABLE();                // 使能 TX 引脚所在 GPIO 端口时钟
        USARTX_RX_GPIO_CLK_ENABLE();                // 使能 RX 引脚所在 GPIO 端口时钟
        gpio_init_struct.Pin = USARTX_TX_GPIO_PIN;  // 选择 TX 引脚
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;    // 设置为复用推挽输出
        gpio_init_struct.Pull = GPIO_PULLUP;        // 设置为上拉
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH; // 设置 GPIO 速度为高速
        gpio_init_struct.Alternate = USARTX_TX_GPIO_AF; // 设置复用功能为 USART1
        HAL_GPIO_Init(USARTX_TX_GPIO_PIN_PORT, &gpio_init_struct); // 初始化 TX 引脚

        gpio_init_struct.Pin = USARTX_RX_GPIO_PIN;  // 选择 RX 引脚
        gpio_init_struct.Alternate = USARTX_RX_AF;  // 设置 RX 引脚复用功能为 USART1
        HAL_GPIO_Init(USARTX_RX_GPIO_PIN_PORT, &gpio_init_struct); // 初始化 RX 引脚

        HAL_NVIC_SetPriority(USARTX_IRQn, 3, 3);    // 设置 USART1 中断优先级，抢占优先级 3，子优先级 3
        HAL_NVIC_EnableIRQ(USARTX_IRQn);            // 使能 USART1 中断

    }                                               // if 结束

}                                                   // HAL_UART_MspInit 函数结束

/**
 * @brief      串口接收完成回调函数
 * @param       uart: UART 句柄指针
 * @note       每收到 1 个字节，硬件触发中断，HAL 库会自动调用此函数
 *             核心逻辑：利用 \r (0x0D) 和 \n (0x0A) 作为一帧数据的结束标志
 *             状态变量 g_usartx_rx_status_data (16位) 被拆成三部分使用：
 *             [bit15]   : 帧接收完成标志 (1=完成, 0=未完成)
 *             [bit14]   : 收到回车符 \r 标志 (1=已收到 \r, 0=未收到)
 *             [bit13~0] : 当前已接收的有效数据字节数 (索引/长度)
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
    // 1. 判断中断来源：确保是 USARTX (例如 USART1) 触发的接收中断
    if (uart->Instance == USARTX)
    {
        // 2. 检查 bit15：如果 bit15 为 0，说明上一帧已经处理完毕（或还没有数据），当前正在接收新的一帧
        if ((g_usartx_rx_status_data & 0x8000) == 0)
        {
            // 3. 检查 bit14：如果 bit14 为 1，说明之前已经收到过回车符 \r 了
            if (g_usartx_rx_status_data & 0x4000)
            {
                // 4. 此时期望收到换行符 \n (0x0A)
                if (g_usart_rx_buffer[0] != 0x0a)
                {
                    // 如果来的不是 \n，说明数据格式错误（例如用户发了 "12\r34"）
                    // 这种情况直接清零状态，丢弃之前收到的所有数据，重新开始接收
                    g_usartx_rx_status_data = 0;
                }
                else
                {
                    // 如果来的正是 \n，说明一帧数据完整结束（格式如 "abc\r\n"）
                    // 将 bit15 置为 1，标记一帧接收完成
                    // 注意：此时低14位的长度不会变，还是之前的长度
                    g_usartx_rx_status_data |= 0x8000;
                }
            }
            // 5. 如果 bit14 为 0，说明之前还没收到过 \r
            else
            {
                // 检查当前字节是不是回车符 \r (0x0D)
                if (g_usart_rx_buffer[0] == 0x0d)
                {
                    // 是 \r，将 bit14 置为 1，标记已收到回车，接下来期待 \n
                    g_usartx_rx_status_data |= 0x4000;
                }
                else
                {
                    // 6. 如果既不是 \r 也不是 \n，说明是普通的数据字节
                    // 取出当前的长度索引（低14位），把数据存入完整帧缓冲区
                    // 例如：当前 g_usartx_rx_status_data 为 0x0002，则存入数组下标 2 的位置
                    g_usartx_rx_buffer_max[g_usartx_rx_status_data & 0x3FFF] = g_usart_rx_buffer[0];

                    // 数据长度 +1 (低14位加1)
                    g_usartx_rx_status_data++;

                    // 7. 防止缓冲区溢出：如果长度超过了数组最大容量
                    if (g_usartx_rx_status_data > (USARTX_RX_BUFFER_SIZE_MAX -1))
                    {
                        // 清空状态，从头开始覆盖旧数据（防数组越界）
                        g_usartx_rx_status_data = 0;
                    }
                }
            }
        }

        // 8. 必须重新开启中断接收！
        // HAL 库的 HAL_UART_Receive_IT 是一次性的，收完一个字节后会自动关闭中断。
        // 所以每处理完一个字节，必须再次调用它，才能继续接收下一个字节。
        HAL_UART_Receive_IT(&g_usartx_handle, (uint8_t *)g_usart_rx_buffer, USARTX_RX_BUFFER_SIZE);
    }
}

void USARTX_IRQHandler(void)                        // USARTX 中断服务函数
{                                                   // 函数体开始

    HAL_UART_IRQHandler(&g_usartx_handle);          // 调用 HAL 库的串口中断处理函数

}                                                   // 中断服务函数结束
