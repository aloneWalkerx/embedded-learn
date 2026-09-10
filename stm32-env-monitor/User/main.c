#include  "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"

int  main (void){
    uint16_t len;
    uint16_t times = 0;
    //初始化HAL库
    HAL_Init();
    //初始化系统时钟(168MHz)
    sys_stm32_clk_init(336, 8, 2, 7);
    //初始化延时
    delay_init(168);
    //初始化串口
    usart_init(115200);



}
