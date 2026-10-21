#include "./SYSTEM/usart/usart.h"
#include "stdio.h"

/////////////////////////////////////////////代码法-不使用半主机模式//////////////////////////////////////////////////
//1，不使用半主机模式
//告诉ARM连接器不要使用半主机模式
#pragma import(__use_no_semihosting)

//自定义FILE结构体，标准库用它来支持fputc
struct __FILE {
    //文件句柄，仅占位
    int handle;

};

//调试终端输出字符函数ARM库调用
int _ttywrch(int ch) {
    //空操作，消除“未使用参数”警告
    ch = ch;
    return ch;
}

//系统退出函数，避免半主机模式下的退出处理
void _sys_exit(int x) {
    //空操作，消除“未使用参数”警告
    x = x;

}

//命令字符串处理函数
char *_sys_command_string(char *cmd, int len) {

    return NULL;

}

//定义标准输出文件对象，给fputc使用
FILE __stdout;

//重定向printf的底层字符输出函数
int fputc(int ch, FILE *f) {
    //等待USARTX发送数据寄存器为空（TXE标志位，bit6 = 0x40）
    while ((CAN_USARTX->SR &0x40) == 0);
    //把字符写入USARTX数据寄存器，发送出去
    CAN_USARTX->DR = (uint8_t)ch;

    return ch;
}

/////////////////////////////////////////////代码法-不使用半主机模式//////////////////////////////////////////////////

//2，全局变量
//UART句柄
UART_HandleTypeDef g_uart_handle;
//定义一个接收数据以及数据状态的变量
uint16_t g_can_usartx_status_data;
//定义单次接收缓冲数组变量
uint8_t g_can_usartx_rx_buffer[CAN_USARTX_RX_BUFFER_SIZE];
//定义最大接收缓冲数组变量
uint8_t g_can_usartx_rx_buffer_max[CAN_USARTX_RX_BUFFER_MAX];

//初始化USARTX
void can_usartx_init(uint32_t baudrate) {
    //具体哪个串口
    g_uart_handle.Instance = CAN_USARTX;
    //波特率
    g_uart_handle.Init.BaudRate = baudrate;
    //数据位
    g_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
    //停止位
    g_uart_handle.Init.StopBits = UART_STOPBITS_1;
    //校验位
    g_uart_handle.Init.Parity = UART_PARITY_NONE;
    //硬件流控
    g_uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    //设置串口收发一体
    g_uart_handle.Init.Mode = UART_MODE_TX_RX;

    //根据参数初始化串口
    HAL_UART_Init(&g_uart_handle);

    //开启接收中断，每次收一字节
    HAL_UART_Receive_IT(&g_uart_handle, g_can_usartx_rx_buffer, CAN_USARTX_RX_BUFFER_SIZE);

}

void HAL_UART_MspInit(UART_HandleTypeDef *uart) {
    //GPIO初始化结构体
    GPIO_InitTypeDef g_struct_init = {0};
    //是否为对应的串口
    if (uart->Instance == CAN_USARTX) {
        //使能发送引脚
        CAN_USARTX_TX_GPIO_CLK_ENABLE();
        //使能接收引脚
        CAN_USARTX_RX_GPIO_CLK_ENABLE();
        //使能串口
        CAN_USARTX_CLK_ENABLE();

        //对应串口的发送引脚
        g_struct_init.Pin = CAN_USARTX_TX_GPIO_PIN;
        //复用推挽输出
        g_struct_init.Mode = GPIO_MODE_AF_PP;
        //上拉
        g_struct_init.Pull = GPIO_PULLUP;
        //速率设置为高速率
        g_struct_init.Speed = GPIO_SPEED_FREQ_HIGH;
        //复用号
        g_struct_init.Alternate = CAN_USARTX_TX_GPIO_AF;
        //根据参数初始化引脚
        HAL_GPIO_Init(CAN_USARTX_TX_GPIO_PORT, &g_struct_init);

        //对应的接收引脚
        g_struct_init.Pin = CAN_USARTX_RX_GPIO_PIN;
        //复用推挽输出
        g_struct_init.Mode = GPIO_MODE_AF_PP;
        //上拉
        g_struct_init.Pull = GPIO_PULLUP;
        //速率设置为高速
        g_struct_init.Speed = GPIO_SPEED_FREQ_HIGH;
        //复用号
        g_struct_init.Alternate = CAN_USARTX_RX_GPIO_AF;
        //根据对应参数初始化接收引脚
        HAL_GPIO_Init(CAN_USARTX_RX_GPIO_PORT, &g_struct_init);

        //设置中断优先级
        HAL_NVIC_SetPriority(CAN_USARTX_IRQn, 3, 3);
        //使能中断
        HAL_NVIC_EnableIRQ(CAN_USARTX_IRQn);

    }

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart) {
    //如果是对应串口
    if (uart->Instance == CAN_USARTX) {
        //如果数据未接收完成
        if ((g_can_usartx_status_data & 0x8000) == 0) {
            //如果上一次已经收到“/r(0x0d)”
            if (g_can_usartx_status_data & 0x4000) {
                //如果当前接收的数据是“/n（0x0a）”
                if (g_can_usartx_rx_buffer[0] == 0x0a) {
                    g_can_usartx_status_data |= 0x8000;

                } else {
                    //重置数据
                    g_can_usartx_status_data = 0;

                }

            }
            //上一个数据没有收到“/r(0x0a)”
            else {

                //如果当前数据是“/r(0x0d)”
                if (g_can_usartx_rx_buffer[0] == 0x0d) {
                    //标记数据状态为已收到“/r”
                    g_can_usartx_status_data |= 0x4000;

                }
                //当前数据不是“/r(即正常数据)”
                else {
                    //把数据存入缓冲区
                    g_can_usartx_rx_buffer_max[g_can_usartx_status_data & 0x3FFF] = g_can_usartx_rx_buffer[0];
                    //数据长度加1
                    g_can_usartx_status_data++;
                    //如果数据超过了最大缓冲大小
                    if (g_can_usartx_status_data > (CAN_USARTX_RX_BUFFER_MAX -1)) {
                        //重置数据
                        g_can_usartx_status_data = 0;

                    }
                }

            }

        }
        // 每接收完一个数据后重新开启接收中断
        HAL_UART_Receive_IT(&g_uart_handle, g_can_usartx_rx_buffer, CAN_USARTX_RX_BUFFER_SIZE);
    }

}

//USARTX中断服务函数
void CAN_USARTX_IRQHandler(void) {
    // 调用 HAL 库的串口中断处理函数
    HAL_UART_IRQHandler(&g_uart_handle);

}
