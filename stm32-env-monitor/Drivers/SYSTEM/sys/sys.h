#ifndef  __ZX_SYS_H
#define  __ZX_SYS_H

#include "stm32f4xx.h"
#include "core_cm4.h"
#include "stm32f4xx_hal.h"

/*
  ZX_SYS_SUPPORT_OS用于定义系统文件夹是否支持OS
  0，不支持OS
  1，支持OS
*/
#define  SYS_SUPPORT_OS          0

//函数声明

    //设置中断偏移量
    void sys_nvic_set_vector_table(uint32_t baseaddr, uint32_t offset);

    //进入待机模式
    void sys_standby_mode(void);

    //系统软复位
    void sys_soft_reset(void);

    //配置系统时钟
    uint8_t sys_stm32_clk_init(uint32_t plln, uint32_t pllm, uint32_t pllp, uint32_t pllq);

//汇编函数声明
    //执行WFI指令
    void sys_wfi_set(void);
    
    //关闭所有中断
    void sys_all_nvic_disable(void);
    
    //开启所有中断
    void sys_all_nvic_enable(void);
    
    //设置栈顶地址
    void sys_set_stack_top_addr(uint32_t addrt);


#endif
