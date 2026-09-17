#include "./SYSTEM/SYS/sys.h"
#include "./BSP/EXTI/exti.h"
#include "./BSP/LED/led.h"
#include "./SYSTEM/DELAY/delay.h"


void exti_init(void)
{
    KEYUP_EXTI_CLK_ENABLE();
    KEY0_EXTI_CLK_ENABLE();
    
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    gpio_init_struct.Pin = KEYUP_EXTI_PIN;
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    
    HAL_GPIO_Init(KEYUP_EXTI_PIN_TYPE, &gpio_init_struct);

    gpio_init_struct.Pin = KEY0_EXTI_PIN;
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    
    HAL_GPIO_Init(KEY0_EXTI_PIN_TYPE, &gpio_init_struct);

    HAL_NVIC_SetPriority(KEYUP_EXTI_IRQn,0, 0);
    HAL_NVIC_EnableIRQ(KEYUP_EXTI_IRQn);
    
    HAL_NVIC_SetPriority(KEY0_EXTI_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(KEY0_EXTI_IRQn);
    
}


void KEYUP_EXTI_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(KEYUP_EXTI_PIN);

}

void KEY0_EXTI_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(KEY0_EXTI_PIN);

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
        delay_ms(20);
    switch(GPIO_Pin){
        case KEYUP_EXTI_PIN:
        {
         LED0_TOGGLE();
         break;
        
        }
        case KEY0_EXTI_PIN:
        {
           LED1_TOGGLE();
            break;
        
        }
    }


}

