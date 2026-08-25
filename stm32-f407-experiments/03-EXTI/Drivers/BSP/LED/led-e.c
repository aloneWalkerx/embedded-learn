#include "./BSP/LED/led-e.h"



//初始化外设载LED
void led_e_init(void)
{
    //1，创建引脚初始化实例
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    //2，使能对应引脚类型的时钟
    //使能LEDR对应引脚类型的时钟
    LEDR_GPIO_CLK_ENABLE();
    
    //使能LEDY对应引脚类型的时钟
   /* 
    注释掉，重复
    LEDY_GPIO_CLK_ENABLE();
    
    //使能LEDG对应引脚类型的时钟
    LEDG_GPIO_CLK_ENABLE();

    */
    
    //3，设置引脚为LEDR
    gpio_init_struct.Pin = LEDR_GPIO_PIN | LEDY_GPIO_PIN |LEDG_GPIO_PIN;
    //设置模式为推挽输出
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    //设置为下拉
    gpio_init_struct.Pull = GPIO_PULLUP;
    //设置速度为低速模式
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    //根据参数初始化LED0引脚
    HAL_GPIO_Init(LEDR_GPIO_PORT, &gpio_init_struct);

/*
    注释掉，代码重复，合三为一，都是同一种引脚类型，GPIOA
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
    
    */
    //6，默认关闭LEDR,LEDY,LEDG
    LEDR(0);
    LEDY(0);
    LEDG(0);
}

//7，开启外设LEDR
void led_R_on(void)
 {
    HAL_GPIO_WritePin(LEDR_GPIO_PORT, LEDR_GPIO_PIN, GPIO_PIN_SET); 
 }
 
//8，关闭外设LEDR
void led_R_off(void)
 {
    HAL_GPIO_WritePin(LEDR_GPIO_PORT, LEDR_GPIO_PIN, GPIO_PIN_RESET); 
 }
 
//9，开启外设LEDY
void led_Y_on(void)
 {
    HAL_GPIO_WritePin(LEDY_GPIO_PORT, LEDY_GPIO_PIN, GPIO_PIN_SET); 
 }
 
//10，关闭外设LEDY
void led_Y_off(void)
 {
    HAL_GPIO_WritePin(LEDY_GPIO_PORT, LEDY_GPIO_PIN, GPIO_PIN_RESET); 
 }
 
//11，开启外设LEDG
void led_G_on(void)
 {
    HAL_GPIO_WritePin(LEDG_GPIO_PORT, LEDG_GPIO_PIN, GPIO_PIN_SET); 
 }
 
//12，关闭外设LEDG
void led_G_off(void)
 {
    HAL_GPIO_WritePin(LEDG_GPIO_PORT, LEDG_GPIO_PIN, GPIO_PIN_RESET); 
 }

