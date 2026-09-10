#include  "./SYSTEM/sys/sys.h"
#include  "./SYSTEM/usart/usart.h"


/* 如果使用os,则包括下面的头文件即可 */
#if SYS_SUPPORT_OS

//os 使用 
#include "os.h"

#endif

/*****************************************代码法*************************************************/
/* 加入以下代码, 支持printf函数, 而不需要选择use MicroLIB */

#if 1

//使用AC6编译器时
#if (__ARMCC_VERSION >= 6010050)

//声明不使用半主机模式
__asm(".global __use_no_semihosting\n\t");

//AC6下需要声明main函数为无参数格式，否则部分例程可能出现半主机模式
__asm(".global __ARM_use_no_argv \n\t");

#else
/* 使用AC5编译器时, 要在这里定义__FILE 和 不使用半主机模式 */
#pragma import(__use_no_semihosting)

struct __FILE
{
    int handle;
    /* Whatever you require here. If the only file you are using is */
    /* standard output using printf() for debugging, no file handling */
    /* is required. */
};

#endif

// 不使用半主机模式，至少需要重定义_ttywrch，_sys_exit，_sys_command_string,,fputc函数,以同时兼容AC6和AC5模式 
int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

/* 定义_sys_exit()以避免使用半主机模式 */
void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}

// FILE 在 stdio.h里面定义
FILE __stdout;

//重定义fputc函数, printf函数最终会通过调用fputc输出字符串到串口
int fputc(int ch, FILE *f)
{
    //等待上一个字符发送完成
    while ((USART1->SR & 0X40) == 0);
    
    //将要发送的字符 ch 写入到DR寄存器
    USART1->DR = (uint8_t)ch;
    
    return ch;
}
#endif
/*****************************************代码法*************************************************/

//如果使能了接收
#if USART_REC_ENABLE_STATUS

/* 接收缓冲, 最大USART_REC_LEN_MAX个字节. */
uint8_t g_usart_rec_buf_max[USART_REC_LEN_MAX];

/*  接收状态
 *  bit15，      接收完成标志
 *  bit14，      接收到0x0d
 *  bit13~0，    接收到的有效字节数目
*/
uint16_t g_usart_rec_status = 0;

//HAL库使用的串口接收缓冲
uint8_t g_hal_rec_buf[HAL_REC_BUF_SIZE];

//UART句柄
UART_HandleTypeDef g_uart_handle_type_def;


/**
 * @brief       串口X初始化函数
 * @param       baudrate: 波特率, 根据自己需要设置波特率值
 * @note        注意: 必须设置正确的时钟源, 否则串口波特率就会设置异常.
 *              这里的USART的时钟源在sys_stm32_clock_init()函数中已经设置过了.
 * @retval      无
 */
void usart_init(uint32_t baudrate)
{
    //USART1
    g_uart_handle_type_def.Instance = USARTX;
    
    //波特率
    g_uart_handle_type_def.Init.BaudRate = baudrate;
    
    //字长为8位数据格式
    g_uart_handle_type_def.Init.WordLength = UART_WORDLENGTH_8B;
    
    //一个停止位
    g_uart_handle_type_def.Init.StopBits = UART_STOPBITS_1;
    
    //无奇偶校验位
    g_uart_handle_type_def.Init.Parity = UART_PARITY_NONE;
    
    //无硬件流控
    g_uart_handle_type_def.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    
    //收发模式
    g_uart_handle_type_def.Init.Mode = UART_MODE_TX_RX;
    
    //根据参数初始化串口
    HAL_UART_Init(&g_uart_handle_type_def);
    
    /* 该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量 */
    HAL_UART_Receive_IT(&g_uart_handle_type_def, (uint8_t *)g_hal_rec_buf, HAL_REC_BUF_SIZE);
}



/**
 * @brief       UART底层初始化函数
 * @param       huart: UART句柄类型指针
 * @note        此函数会被HAL_UART_Init()调用
 *              完成时钟使能，引脚配置，中断配置
 * @retval      无
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    //引脚初始化句柄
    GPIO_InitTypeDef gpio_init_struct;
    
    //如果是串口1，进行串口1的 MSP初始化
    if(huart->Instance == USARTX)
    {
        //USART1 时钟使能
        USARTX_CLK_ENABLE();
        
        //发送引脚时钟使能
        USART_TX_GPIO_CLK_ENABLE();
        
        //接收引脚时钟使能
        USART_RX_GPIO_CLK_ENABLE();
    //TX发送引脚定义
        //TX发送引脚号
        gpio_init_struct.Pin = USART_TX_GPIO_PIN;
        
        //复用推挽输出
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        
        //上拉
        gpio_init_struct.Pull = GPIO_PULLUP;
        
        //高速
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        
        //引脚复用为USART1
        gpio_init_struct.Alternate = USART_TX_GPIO_AF;
        
        //根据参数初始化发送引脚
        HAL_GPIO_Init(USART_TX_GPIO_PIN_TYPE, &gpio_init_struct);

    //RX接收引脚定义
        //RX接收引脚号
        gpio_init_struct.Pin = USART_RX_GPIO_PIN;
        
        //复用为USART1
        gpio_init_struct.Alternate = USART_RX_GPIO_AF;
        
        //根据参数初始化接收引脚
        HAL_GPIO_Init(USART_RX_GPIO_PIN_TYPE, &gpio_init_struct);

//如果开启了串口接收
#if USART_REC_ENABLE_STATUS
        
        //使能USART1中断通道 
        HAL_NVIC_EnableIRQ(USARTX_IRQn);
        
        //抢占优先级3，子优先级3
        HAL_NVIC_SetPriority(USARTX_IRQn, 3, 3);
#endif
    }
}

/**
 * @brief       Rx传输回调函数
 * @param       huart: UART句柄类型指针
 * @retval      无
    g_hal_rec_buf[0] ：当前要接收的数据

    bit15	0x8000	接收完成标志	1 = 已收到 \r\n，数据完整 0 = 正在接收中
    
    bit14	0x4000	已收到 \r 标志	1 = 已经收到过 \r 0 = 还没收到 \r
    
    bit13 ~ bit0	0x3FFF	已接收的有效字节数	0 ~ 16383（最大可存 16383 字节）

    接收数据简化流程：
    硬件自动接收，存入 USART1->DR 寄存器。

    触发中断，HAL 库自动把 DR 寄存器的值搬运到 g_hal_rec_buf[0]（在usart_i_init中已定义g_hal_rec_buf[0]）

    然后调用回调函数 HAL_UART_RxCpltCallback。

 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    //如果寄存器地址是USART1,则执行以下代码
    if(huart->Instance == USARTX)
    {
        //上一次或初次数据状态，接收未完成,继续接收数据
        if((g_usart_rec_status & 0x8000) == 0)
        {
            //如果上一次或初次数据状态是0x0d(\r)
            if(g_usart_rec_status & 0x4000)
            {
                //0x0a ： \n
                //如果当前要接收的数据不是“\n”说明数据格式错误，正常的数据格式是“123\r\n”,此处是错误格式，比如：“12\r34”,重新开始 
                if(g_hal_rec_buf[0] != 0x0a) 
                {
                    //丢弃此前数据，重新开始接收数据
                    g_usart_rec_status = 0;
                }
                
                //如果收到的是“\n”说明整条数据正常结束了
                else 
                {
                    //将状态变量的第 15 位（最高位）置 1，标记“一帧数据接收完成”。
                    g_usart_rec_status |= 0x8000;
                }
            }
            
            //还没收到0X0D，即没有收到“\r”
            else
            {
                //0x0d : \r
                //如果当前要接收的数据是“\r”
                if(g_hal_rec_buf[0] == 0x0d)
                {
                    //将 g_usart_i_rx_status 的第 14 位（bit14）置为 1，同时保持其他位不变
                    //告诉程序：“回车符已经来了，下一个字节我期待收到 \n。”
                    g_usart_rec_status |= 0x4000;
                }
                else
                {
                    //g_usart_i_rx_status & 0x3FFF ：当前要写入的位置索引
                    //当前要写入的数据
                    g_usart_rec_buf_max[g_usart_rec_status & 0X3FFF] = g_hal_rec_buf[0] ;
                    
                    //位置索引加1
                    g_usart_rec_status++;
                    
                    // 如果长度超过了缓冲区最大值，从头开始（防止数组越界）,防止溢出
                    if(g_usart_rec_status > (USART_REC_LEN_MAX - 1))
                    {
                        //接收数据错误,重新开始接收,覆盖旧数据
                        g_usart_rec_status = 0;
                    }
                }
            }
        }
        /*
        此次数据已经接收完成并启动下一次中断接收,每收到 1 个字节，中断触发，调用这个回调
        处理完后，必须再次调用它，才能继续接收下一个字节,如果不调用，串口只能收到 1 个字节就停止工作了。
        就像循环一样，所以在初始化上面“usart_i_init”函数的时候也这样调用这个方法
        */
        HAL_UART_Receive_IT(&g_uart_handle_type_def, (uint8_t *)g_hal_rec_buf, HAL_REC_BUF_SIZE);
    }
}

/**
 * @brief       串口1中断服务函数
 * @param       无
 * @retval      无
 *多个串口需要定义多个IRQHandler，然后将对应的handle实例传入HAL_UART_IRQHandler函数中
 */
void USARTX_IRQHandler(void)
{
//使用OS
#if SYS_SUPPORT_OS
    OSIntEnter();    
#endif
    
    //调用HAL库中断处理公用函数
    HAL_UART_IRQHandler(&g_uart_handle_type_def);

//使用OS
#if SYS_SUPPORT_OS
    OSIntExit();
#endif
}

#endif

