#ifndef LED_E_H
#define LED_E_H

#include "./SYSTEM/sys/sys.h"

//1，定义红色LED灯的引脚

//定义外设LED灯引脚的类型
#define     LEDR_GPIO_PORT                  GPIOA

//定义外设LED灯引脚对应的引脚号
#define     LEDR_GPIO_PIN                   GPIO_PIN_1

//使能外设LED引脚对应的时钟
#define     LEDR_GPIO_CLK_ENABLE()          do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)


//2，定义红色LED灯的引脚

//定义外设LED灯引脚的类型
#define     LEDY_GPIO_PORT                  GPIOA

//定义外设LED灯引脚对应的引脚号
#define     LEDY_GPIO_PIN                   GPIO_PIN_2

//使能外设LED引脚对应的时钟
#define     LEDY_GPIO_CLK_ENABLE()          do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)


//3，定义红色LED灯的引脚

//定义外设LED灯引脚的类型
#define     LEDG_GPIO_PORT                  GPIOA

//定义外设LED灯引脚对应的引脚号
#define     LEDG_GPIO_PIN                   GPIO_PIN_3

//使能外设LED引脚对应的时钟
#define     LEDG_GPIO_CLK_ENABLE()          do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)


//3，对外设LEDR引脚进行写操作（x为1则赋值为1，x不为1则赋值为0）
#define     LEDR(x)                         do {  (x) ?                                                                  \
                                                  HAL_GPIO_WritePin(LEDR_GPIO_PORT, LEDR_GPIO_PIN, GPIO_PIN_SET) :         \
                                                  HAL_GPIO_WritePin(LEDR_GPIO_PORT, LEDR_GPIO_PIN, GPIO_PIN_RESET);       \
                                                } while (0)

//4，对外设LEDY引脚进行写操作（x为1则赋值为1，x不为1则赋值为0）
#define     LEDY(x)                         do {  (x) ?                                                                  \
                                                  HAL_GPIO_WritePin(LEDY_GPIO_PORT, LEDY_GPIO_PIN, GPIO_PIN_SET) :         \
                                                  HAL_GPIO_WritePin(LEDY_GPIO_PORT, LEDY_GPIO_PIN, GPIO_PIN_RESET);       \
                                                } while (0)                                                
                                                

//5，对外设LEDY引脚进行写操作（x为1则赋值为1，x不为1则赋值为0）
#define     LEDG(x)                         do {  (x) ?                                                                  \
                                                  HAL_GPIO_WritePin(LEDG_GPIO_PORT, LEDG_GPIO_PIN, GPIO_PIN_SET) :         \
                                                  HAL_GPIO_WritePin(LEDG_GPIO_PORT, LEDG_GPIO_PIN, GPIO_PIN_RESET);       \
                                                } while (0)  

//6，初始化外设LED
void led_e_init(void);                                                
                                                

#endif