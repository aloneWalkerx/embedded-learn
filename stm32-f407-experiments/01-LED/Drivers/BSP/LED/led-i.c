#include "./BSP/LED/led-i.h"


//初始化板载LED
void led_i_init(void)
{
    //1，创建引脚初始化实例
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    //2，使能对应引脚类型的时钟
    //使能LED0对应引脚类型的时钟
    LED0_GPIO_CLK_ENABLE();
    //使能LED1对应引脚类型的时钟
    LED1_GPIO_CLK_ENABLE();
    
    //3，设置引脚为LED0
    gpio_init_struct.Pin = LED0_GPIO_PIN;
    //设置模式为推挽输出
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    //设置为下拉
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    //设置速度为低速模式
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    //根据参数初始化LED0引脚
    HAL_GPIO_Init(LED0_GPIO_PORT, &gpio_init_struct);
    
    
    //4，设置引脚为LED1
    gpio_init_struct.Pin = LED1_GPIO_PIN;
    //设置模式为推挽输出
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    //设置为下拉
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    //设置速度为低速模式
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    //根据参数初始化LED0引脚
    HAL_GPIO_Init(LED1_GPIO_PORT, &gpio_init_struct);
    
    //,5，默认关闭LED0、LED1
    LED0(1);
    LED1(1);
    
}

void led_0_on(void)
 {
    HAL_GPIO_WritePin(LED0_GPIO_PORT, LED0_GPIO_PIN, GPIO_PIN_RESET); 
 }

void led_0_off(void)
 {
    HAL_GPIO_WritePin(LED0_GPIO_PORT, LED0_GPIO_PIN, GPIO_PIN_SET); 
 }
void led_1_on(void)
  {
    HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_RESET); 
  }
void led_1_off(void)
  {
    HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_SET); 
 }


