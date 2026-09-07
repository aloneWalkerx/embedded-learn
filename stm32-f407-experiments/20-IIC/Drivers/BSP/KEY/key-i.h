#ifndef __KEY_I_H
#define __KEY_I_H

#include "./SYSTEM/sys/sys.h"

//1，引脚定义

//定义WKUP按键的类型为GPIOA
#define     WKUP_GPIO_PORT              GPIOA

//定义WKUP的引脚号为0
#define     WKUP_GPIO_PIN               GPIO_PIN_0

//使能WKUP引脚的时钟
#define     WKUP_GPIO_CLK_ENABLE()        do { __HAL_RCC_GPIOA_CLK_ENABLE();} while (0)


//定义KEY0按键的类型为GPIOE
#define     KEY0_GPIO_PORT              GPIOE

//定义KEY0的引脚号为0
#define     KEY0_GPIO_PIN               GPIO_PIN_4

//使能KEY0引脚的时钟
#define     KEY0_GPIO_CLK_ENABLE()        do { __HAL_RCC_GPIOE_CLK_ENABLE();} while (0)

//2，读取引脚状态

//读取WKUP是否按下
#define     WKUP_STATUS                 ((HAL_GPIO_ReadPin(WKUP_GPIO_PORT, WKUP_GPIO_PIN) == GPIO_PIN_SET) ? 1 : 0 )

//读取KEY0是否按下
#define     KEY0_STATUS                 ((HAL_GPIO_ReadPin(KEY0_GPIO_PORT, KEY0_GPIO_PIN) == GPIO_PIN_SET) ? 1: 0 )      

//3，键值定义
//没有任何按键按下
#define     NONE_PRESS                  0

//按下WKUP
#define     WKUP_PRESS                  1

//按下KEY0
#define     KEY0_PRESS                  2



//4，函数声明
//初始化按键
void key_i_init(void);

//扫描按键
uint8_t key_i_scan(uint8_t mode);

#endif




