#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/TIMER/g-timer.h"

int main(void)
{ 
    
    /* 定时器输入捕获相关变量 */
    extern uint8_t g_timx_chy_cap_sta;
    extern uint16_t g_timx_chy_cap_val;
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    
    while (1)
    {
     
    uint32_t total = 0;
    uint8_t t = 0;
    
    HAL_Init();                                 /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7);         /* 配置时钟，168MHz */
    delay_init(168);                            /* 初始化延时 */
    usart_init(115200);                         /* 初始化串口 */
    led_i_init();                                 /* 初始化LED */
    gtim_timx_cap_chy_init(0xFFFF, 84 - 1);     /* 初始化通用定时器输入捕获，捕获频率为1MHz，0xFFFF = 65535 */
    
    while (1)
    {
        if ((g_timx_chy_cap_sta & 0x80) != 0)   /* 捕获完成 */
        {
            //总时间 = (溢出次数 × 65535) + 最后一次计数值
            //提取“溢出次数”
            total = g_timx_chy_cap_sta & 0x3F;
            //计算“已完整计满的周期”所对应的总时间
            total *= 0xFFFF;
            //加上最后一次“没计满”的那部分时间
            total += g_timx_chy_cap_val;
            printf("High: %dus\r\n", total);
            g_timx_chy_cap_sta = 0;             /* 开启下一次输入捕获 */
        }
        
        if (++t == 20)
        {
            t = 0;
            LED0_TOGGLE();
        }
        
        delay_ms(10);
    }   
      
        
        
        
    }
}
