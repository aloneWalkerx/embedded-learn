#include "./SYSTEM/SYS/sys.h"
#include "./BSP/LED/led.h"


void led_init(void)
{
 LED0_GPIO_CLK_ENABLE();
 LED1_GPIO_CLK_ENABLE();
 
 GPIO_InitTypeDef gpio_init_type_def = {0};
 
 gpio_init_type_def.Pin = LED0_GPIO_PIN | LED1_GPIO_PIN;
 gpio_init_type_def.Mode = GPIO_MODE_OUTPUT_PP;
 gpio_init_type_def.Pull = GPIO_PULLDOWN;
 gpio_init_type_def.Speed = GPIO_SPEED_FREQ_LOW;
 
 HAL_GPIO_Init(LED0_GPIO_PIN_TYPE, &gpio_init_type_def);

 LED0_OFF();
 LED1_OFF();

}
