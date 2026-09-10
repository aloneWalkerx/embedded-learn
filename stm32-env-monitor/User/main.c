#include  "./SYSTEM/sys/sys.h"



int  main (void){

    //初始化HAL库
    HAL_Init();
    //初始化系统时钟(168MHz)
    sys_stm32_clk_init(336, 8, 2, 7);



return 0;
}

