#include "./SYSTEM/SYS/sys.h"

#include "./BSP/LED/led.h"



void led_init(void)
{

 LED0_GPIO_CLK_ENABLE();


 LED1_GPIO_CLK_ENABLE();

 GPIO_InitTypeDef gpio_init_struct = {0};

 gpio_init_struct.Pin = LED0_GPIO_PIN | LED1_GPIO_PIN;
 

 gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
 

 gpio_init_struct.Pull = GPIO_NOPULL;
 

 gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
 
 
 HAL_GPIO_Init(LED0_GPIO_PIN_TYPE, &gpio_init_struct);

 LED1_OFF();

}
