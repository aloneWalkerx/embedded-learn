#include  "./BSP/EXTI/exti-e.h"
#include  "./SYSTEM/sys/sys.h"
#include  "./SYSTEM/delay/delay.h"
#include  "./BSP/LED/led-e.h"


/*
    
    初始化外设按键控制外设LED中断

*/
void exti_e_init(void)
{
    //1，创建引脚初始化实例
    GPIO_InitTypeDef  gpio_init_struct = {0};
    
    //2，使能外设按键输入引脚
    KEY_R_EXT_GPIO_CLK_ENABLE();
//    KEY_Y_EXT_GPIO_CLK_ENABLE();  //注释掉，重复，都是同一种引脚类型
//    KEY_G_EXT_GPIO_CLK_ENABLE();  //注释掉，重复，都是同一种引脚类型
    
    //3，配置外设按键对应引脚相关参数
    //配置KEY_R外设按脚对应的引脚
    gpio_init_struct.Pin = KEY_R_EXT_GPIO_PIN;
    
    //设置为下降沿
    gpio_init_struct.Mode = GPIO_MODE_IT_FALLING;
    
    //设置为上拉
    gpio_init_struct.Pull = GPIO_PULLUP;
    
    //设置速度为低速
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    
    //根据参数初始化外设按键对应的KEY_R引脚
    HAL_GPIO_Init(KEY_R_EXT_GPIO_PORT, &gpio_init_struct);
    
    //配置KEY_Y外设按脚对应的引脚
    gpio_init_struct.Pin = KEY_Y_EXT_GPIO_PIN;
    //根据参数初始化外设按键对应的KEY_Y引脚
    HAL_GPIO_Init(KEY_Y_EXT_GPIO_PORT, &gpio_init_struct);
    
    //配置KEY_G外设按脚对应的引脚
    gpio_init_struct.Pin = KEY_G_EXT_GPIO_PIN;
    //根据参数初始化外设按键对应的KEY_G引脚
    HAL_GPIO_Init(KEY_G_EXT_GPIO_PORT, &gpio_init_struct);
    
    
    //3，配置中断优先级并使能中断
    //三个中断共用一个中断号，取期中一个即可，因为都是一样的
    HAL_NVIC_SetPriority(KEY_R_EXT_IRQn, 0 , 0);
    HAL_NVIC_EnableIRQ(KEY_R_EXT_IRQn);
    
}

/**
由于 EXTI10、EXTI11、EXTI12 都使用同一个中断服务函数 EXTI15_10_IRQHandler，
需要统一调用 HAL 库的处理函数

*/

void KEY_R_EXT_IRQHandler(void)
{   
    //中断服务函数
    HAL_GPIO_EXTI_IRQHandler(KEY_R_EXT_GPIO_PIN);
    HAL_GPIO_EXTI_IRQHandler(KEY_Y_EXT_GPIO_PIN);
    HAL_GPIO_EXTI_IRQHandler(KEY_G_EXT_GPIO_PIN);
}


/**
HAL库外部中断回调函数
@param  GPIO_Pin : 外部中断线对应的引脚

*/
void HAL_GPIO_EXTI_Callback(uint16_t  GPIO_Pin)
{

    delay_ms(20);//机械按键消抖（仅演示，切勿在实际工程的中断服务函数种进行阻塞延时）
    switch (GPIO_Pin)
    {
        case KEY_R_EXT_GPIO_PIN: //KEY_R按键按下对应引脚发生中断
        {
             LED_R_TOGGLE();     //翻转LED_R状态
             break;
        }
        
        case KEY_Y_EXT_GPIO_PIN:  //KEY_R按键按下对应引脚发生中断
        {
             LED_Y_TOGGLE();      //翻转LED_Y状态
             break;
        }
        
        case KEY_G_EXT_GPIO_PIN:  //KEY_R按键按下对应引脚发生中断
        {
             LED_G_TOGGLE();      //翻转LED_G状态
             break;
        }
    
    }

}
