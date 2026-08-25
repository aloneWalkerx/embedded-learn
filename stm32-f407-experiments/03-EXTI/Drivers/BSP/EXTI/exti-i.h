#ifndef  EXTI_I_H
#define  EXTI_I_H


//WKUP引脚类型
#define WKUP_EXT_GPIO_PORT             GPIOA

//WKUP对应引脚号
#define WKUP_EXT_GPIO_PIN              GPIO_PIN_0

//使能WKUP引脚对应的时钟
#define WKUP_EXT_GPIO_CLK_ENABLE()     do {  __HAL_RCC_GPIOA_CLK_ENABLE(); } while(0)

//WKUP中断号
#define WKUP_EXT_IRQn                  EXTI0_IRQn

//WKUP中断处理函数
#define WKUP_EXT_IRQHandler             EXTI0_IRQHandler

//KEY0引脚类型
#define KEY0_EXT_GPIO_PORT             GPIOE

//KEY0_对应引脚号
#define KEY0_EXT_GPIO_PIN              GPIO_PIN_4

//使能KEY0引脚对应的时钟
#define KEY0_EXT_GPIO_CLK_ENABLE()     do {  __HAL_RCC_GPIOE_CLK_ENABLE(); } while(0)

//KEY0中断号
#define KEY0_EXT_IRQn                  EXTI4_IRQn

//KEY0中断处理函数
#define KEY0_EXT_IRQHandler            EXTI4_IRQHandler

//初始化板载按键控制LED中断
void exti_i_init(void);

#endif
