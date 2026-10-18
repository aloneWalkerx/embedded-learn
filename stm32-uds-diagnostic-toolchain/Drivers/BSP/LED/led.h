#ifndef __CAN_LED_H
#define __CAN_LED_H

#include "./SYSTEM/sys/sys.h"

//STM32F407ZGT6

//LED0引脚类
#define  CAN_LED0_GPIO_PORT              GPIOF
//LED0引脚号
#define  CAN_LED0_GPIO_PIN               GPIO_PIN_9
//LED0引脚使能
#define  CAN_LED0_GPIO_CLK_ENABLE()      do{__HAL_RCC_GPIOF_CLK_ENABLE();}while(0)

//LED1引脚类
#define  CAN_LED1_GPIO_PORT              GPIOF
//LED1引脚号
#define  CAN_LED1_GPIO_PIN               GPIO_PIN_10
//LED1引脚使能
#define  CAN_LED1_GPIO_CLK_ENABLE()      do{__HAL_RCC_GPIOF_CLK_ENABLE();}while(0)

//LED0关闭
#define  CAN_LED0_ON()                     HAL_GPIO_WritePin(CAN_LED0_GPIO_PORT,CAN_LED0_GPIO_PIN,GPIO_PIN_RESET);
//LED0开启
#define  CAN_LED0_OFF()                      HAL_GPIO_WritePin(CAN_LED0_GPIO_PORT,CAN_LED0_GPIO_PIN,GPIO_PIN_SET);

//LED1关闭
#define  CAN_LED1_ON()                     HAL_GPIO_WritePin(CAN_LED1_GPIO_PORT,CAN_LED1_GPIO_PIN,GPIO_PIN_RESET);
//LED1开启
#define  CAN_LED1_OFF()                      HAL_GPIO_WritePin(CAN_LED1_GPIO_PORT,CAN_LED1_GPIO_PIN,GPIO_PIN_SET);

//LED初始化
void can_led_init(void);


#endif
