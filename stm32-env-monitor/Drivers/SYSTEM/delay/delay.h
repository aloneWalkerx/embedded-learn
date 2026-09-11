#ifndef  __DELAY_H
#define  __DELAY_H

#include  "./SYSTEM/sys/sys.h"


//函数声明
    //初始化延时函数
    void delay_init(uint16_t sysclk);
    
    //延时ms
    void delay_ms(uint16_t ms);
    
    //延时ns
    void delay_ns(uint16_t us);

//不支持OS
#if (!ZX_SYS_SUPPORT_OS)
    //HAL库的延时函数，，SDIO等需要用到
    void HAL_Delay(uint32_t Delay);
#endif

#endif  

