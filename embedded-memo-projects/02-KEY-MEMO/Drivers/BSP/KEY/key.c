#include "./SYSTEM/SYS/sys.h"
#include "./BSP/KEY/key.h"
#include "./SYSTEM/DELAY/delay.h"



void key_init(void)
{
    KEYUP_GPIO_CLK_ENABLE();
    KEY0_GPIO_CLK_ENABLE();
    
    GPIO_InitTypeDef  gpio_init_struct = {0};
    
    gpio_init_struct.Pin = KEYUP_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    
    HAL_GPIO_Init(KEYUP_GPIO_PIN_TYPE, &gpio_init_struct);
    
    gpio_init_struct.Pin = KEY0_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    
    HAL_GPIO_Init(KEY0_GPIO_PIN_TYPE, &gpio_init_struct);

}

uint8_t key_scan(uint8_t keyMode)
{  
    static uint8_t key_release_status = 1;
    uint8_t key_value = NONE_PRESS;
    
    if(keyMode !=0){
    key_release_status = 1;
    
    }
    if((key_release_status == 1) &&((READ_KEYUP_PIN == 1) || (READ_KEY0_PIN == 1))){
        delay_ms(10);
        key_release_status = 0;
    if(READ_KEYUP_PIN ==1){
     key_value = KEYUP_PRESS;
    
    }
    if(READ_KEY0_PIN == 1){
     key_value = KEY0_PRESS;
    }
    
    }
    else if ((READ_KEYUP_PIN ==0) && (READ_KEY0_PIN == 0)){
    key_release_status = 1;
    }

    return key_value;
}
