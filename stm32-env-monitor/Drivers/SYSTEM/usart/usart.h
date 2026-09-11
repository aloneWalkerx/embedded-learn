#ifndef  __USART_H
#define  __USART_H

#include  "./stdio.h"
#include  "./SYSTEM/sys/sys.h"


//引脚，串口定义，当前默认是USART1,支持USART1~USART7的任意一个串口
    //发送引脚定义
        //引脚类型
        #define  USART_TX_GPIO_PIN_TYPE                  GPIOA
        
        //引脚号
        #define  USART_TX_GPIO_PIN                       GPIO_PIN_9
        
        //复用号
        #define  USART_TX_GPIO_AF                        GPIO_AF7_USART1
        
        //使能发送引脚
        #define  USART_TX_GPIO_CLK_ENABLE()              do {__HAL_RCC_GPIOA_CLK_ENABLE();}while(0)

    //接收引脚定义
        //引脚类型
        #define  USART_RX_GPIO_PIN_TYPE                  GPIOA
        
        //引脚号
        #define  USART_RX_GPIO_PIN                       GPIO_PIN_10
        
        //复用号
        #define  USART_RX_GPIO_AF                        GPIO_AF7_USART1
        
        //使能发送引脚
        #define  USART_RX_GPIO_CLK_ENABLE()              do {__HAL_RCC_GPIOA_CLK_ENABLE();}while(0)
        
    //串口定义
        //串口号
        #define  USARTX                                  USART1
        
        //中断函数
        #define  USARTX_IRQn                             USART1_IRQn
        
        //中断服务函数
        #define  USARTX_IRQHandler                       USART1_IRQHandler 
        
        //使能串口时钟
        #define  USARTX_CLK_ENABLE()                     do {__HAL_RCC_USART1_CLK_ENABLE();}while(0)
        
//串口参数设置
        //串口最大接收字节数
        #define  USART_REC_LEN_MAX                       200
            
        //串口接收状态(1：开启接收，0：关闭接收)
        #define  USART_REC_ENABLE_STATUS                 1
            
        //接收缓存大小
        #define  HAL_REC_BUF_SIZE                        1            

//外部声明
        //USART句柄
        extern  UART_HandleTypeDef  g_uart_handle_type_def;
        
        //最大接收缓冲，最大USART_REC_LEN_MAX个字节，末字节为换行符
        extern  uint8_t  g_usart_rec_buf_max[USART_REC_LEN_MAX];
        
        //接收状态标记
        extern  uint16_t  g_usart_rec_status;
        
        //HAL库USART接收Buffer
        extern  uint8_t  g_hal_rec_buf[HAL_REC_BUF_SIZE];
        
//函数声明
        //串口初始化
        void usart_init(uint32_t baudrate);
        
#endif

