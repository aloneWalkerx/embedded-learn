#include "./SYSTEM/SYS/sys.h"
#include "./BSP/LED/led.h"
#include "./SYSTEM/DELAY/delay.h"


int main(void)
{
   HAL_Init();
   sys_stm32_clk_init(336,8,2,7);
   delay_init(168);
   led_init();
    
   while(1)
    {
      LED0_ON();
      LED1_OFF();
      delay_ms(500);
      LED0_OFF();
      LED1_ON();
      delay_ms(500);
        

    }



}
