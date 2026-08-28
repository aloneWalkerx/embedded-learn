#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/KEY/key-i.h"
#include "./BSP/TIMER/g-timer.h"
int main(void)
{ 
    uint8_t key;
    uint32_t count;
    uint32_t count_prev = 0;
    uint8_t t = 0;
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                                 /* 初始化LED */
    key_i_init();                                 /* 初始化按键 */
    gtim_timx_cnt_chy_init(0);                  /* 初始化通用定时器脉冲计数 */
    gtim_timx_cnt_chy_restart();                /* 重启通用定时器脉冲计数 */
    while (1)
    {
      
        key = key_i_scan(0);
        if (key == KEY0_PRESS)
        {
            gtim_timx_cnt_chy_restart();        /* 重启通用定时器脉冲计数 */
        }
        
        count = gtim_timx_cnt_chy_get_count();  /* 获取通用定时器脉冲计数值 */
        if (count_prev != count)                /* 脉冲计数值有更新 */
        {
            count_prev = count;
            printf("Cnt: %d\r\n", count);
        }
        
        if (++t == 20)
        {
            t = 0;
            LED0_TOGGLE();
        }
        
        delay_ms(10);  
      
        
        
        
    }
}
