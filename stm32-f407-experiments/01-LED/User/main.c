#include "./SYSTEM/sys/sys.h" 
#include "./SYSTEM/delay/delay.h" 
#include "./SYSTEM/usart/usart.h" 
#include "./BSP/LED/led-i.h"
#include "./BSP/LED/led-e.h"
int main(void) 
{ 
    /*
    STM32 官方 HAL 库的核心初始化函数，是整个软件运行环境的“总开关”或“奠基仪式”。
    在main 函数中必须是第一个被调用的 HAL 库函数
    
    作用：
        1，配置 Flash 预取与缓存：
            它配置 Flash 的预取缓冲区以及指令和数据缓存。能提高 CPU 从 Flash 读取指令和数据的效率，让代码运行更快。

        2，初始化 SysTick 定时器：
            它将系统滴答定时器（SysTick）配置为每 1 毫秒产生一次中断。这个 1ms 的“心跳”是 HAL_Delay() 等延时函数正常工作的基础。
            在时钟系统配置之前，SysTick 的时钟源是内部高速时钟 (HSI)。

        3，设置中断优先级分组：
            它将中断优先级分组设置为 4 位抢占优先级，0 位子优先级（即 NVIC_PriorityGroup_4）。这定义了系统中所有中断的优先级管理规则。

        4，调用 HAL_MspInit() 回调函数：
            最后，它会调用一个弱定义的（weak）回调函数 HAL_MspInit()。这是一个“钩子”函数，允许你在其中进行一些最底层、最通用的硬件初始化，
            比如调试接口、全局时钟等。你可以在用户文件（如 stm32f4xx_hal_msp.c）中重写它，加入自己的底层配置
    
    */
    HAL_Init();
    
    //配置时钟，STM32F407ZGT6为168MHz
    sys_stm32_clock_init(336, 8, 2, 7); 
    
    //初始化延时
    delay_init(168);
    
    //初始化串口
    usart_init(115200);
    
    //初始化板载LED
    led_i_init();
    
    //初始化外设LED
    led_e_init();
    
    while (1)
    {   
        //新版LED状态
        // 开启板载LED0
        led_0_on();
        //开启外设LEDR
        led_R_on();
        //关闭板载LED1
        led_1_off();
        //关闭外设LEDY
        led_Y_off();
        //关闭外设LEDG
        led_G_off();
        //延时500毫秒
        delay_ms(500);
        //关闭板载LED0
        led_0_off();
        //关闭外设LEDR
        led_R_off();
        //开启板载LED1
        led_1_on();
        //开启外设LEDY
        led_Y_on();
        //延时500毫秒
        delay_ms(500);
        //关闭外设LEDY
        led_Y_off();
        //开启外设LEDG
        led_G_on();
        //延时500毫秒
        delay_ms(500);
         //关闭外设LEDG
        led_G_off();
        //延时500毫秒
        delay_ms(500);
        
        
        /*
        //旧版LED状态：
       // 0和1可读性太差，本实验有板载LED以及外设LED且开启关闭状态不一样
        // 开启板载LED0
        LED0(0);
        //开启外设LEDR
        LEDR(1);
        //关闭板载LED1
        LED1(1);
        //关闭外设LEDY
        LEDY(0);
        //关闭外设LEDG
        LEDG(0);
        //延时500毫秒
        delay_ms(500);
        //关闭板载LED0
        LED0(1);
        //关闭外设LEDR
        LEDR(0);
        //开启板载LED1
        LED1(0);
        //开启外设LEDY
        LEDY(1);
        //延时500毫秒
        delay_ms(500);
        //关闭外设LEDY
        LEDY(0);
        //开启外设LEDG
        LEDG(1);
        //延时500毫秒
        delay_ms(500);
         //关闭外设LEDG
        LEDG(1);
        //延时500毫秒
        delay_ms(500);
        */
    }
    
} 
