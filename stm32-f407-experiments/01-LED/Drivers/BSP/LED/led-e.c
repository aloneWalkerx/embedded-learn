#include "./BSP/LED/led-e.h"



//初始化外设载LED
void led_e_init()
{
    //1，创建引脚初始化实例
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    //2，使能对应引脚类型的时钟
    //使能LEDR对应引脚类型的时钟
    LEDR_GPIO_CLK_ENABLE();
    
    //使能LEDY对应引脚类型的时钟
    LEDY_GPIO_CLK_ENABLE();
    
    //使能LEDG对应引脚类型的时钟
    LEDG_GPIO_CLK_ENABLE();

    
    
    //3，设置引脚为LEDR
    gpio_init_struct.Pin = LEDR_GPIO_PIN;
    //设置模式为推挽输出
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    //设置为下拉
    gpio_init_struct.Pull = GPIO_PULLUP;
    //设置速度为低速模式
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    //根据参数初始化LED0引脚
    HAL_GPIO_Init(LEDR_GPIO_PORT, &gpio_init_struct);


    //4，设置引脚为LEDY
    gpio_init_struct.Pin = LEDY_GPIO_PIN;
    //设置模式为推挽输出
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    //设置为下拉
    gpio_init_struct.Pull = GPIO_PULLUP;
    //设置速度为低速模式
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    //根据参数初始化LED0引脚
    HAL_GPIO_Init(LEDY_GPIO_PORT, &gpio_init_struct);
    
    
    //5，设置引脚为LEDY
    gpio_init_struct.Pin = LEDG_GPIO_PIN;
    //设置模式为推挽输出
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    //设置为下拉
    gpio_init_struct.Pull = GPIO_PULLUP;
    //设置速度为低速模式
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    //根据参数初始化LED0引脚
    HAL_GPIO_Init(LEDG_GPIO_PORT, &gpio_init_struct);
    
    //6，默认关闭LEDR,LEDY,LEDG
    LEDR(0);
    LEDR(0);
    LEDR(0);
}