#include "BSP/LED/led.h"

//初始化LED
void can_led_init(void){
    //LED0,LED1引脚类型是一样的，冗余，便于直接替换
    //使能LED0
    CAN_LED0_GPIO_CLK_ENABLE();
    //使能LED1
    CAN_LED1_GPIO_CLK_ENABLE();
    
    //引脚初始化句柄
    GPIO_InitTypeDef   g_gpio_init = {0};
    //初始化的对应引脚号
    g_gpio_init.Pin = CAN_LED0_GPIO_PIN | CAN_LED1_GPIO_PIN;
    //推挽输出
    g_gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    //无上下拉
    g_gpio_init.Pull = GPIO_NOPULL;
    //低输出速度
    g_gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
    
    //根据对应参数初始化引脚
    HAL_GPIO_Init(CAN_LED0_GPIO_PORT, &g_gpio_init);
    
    //LED0默认关闭
    CAN_LED0_OFF();
    //LED1默认关闭
    CAN_LED1_OFF();
   
}
