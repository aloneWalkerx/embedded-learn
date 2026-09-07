#ifndef  __KEY_E_H
#define  __KEY_E_H

#include  "./SYSTEM/sys/sys.h"
//1，引脚定义

//定义KEYR按键的类型为GPIOB
#define     KEY_R_GPIO_PORT              GPIOB

//定义KEYR的引脚号为10
#define     KEY_R_GPIO_PIN               GPIO_PIN_10

//使能KEYR引脚的时钟
#define     KEY_R_GPIO_CLK_ENABLE()        do { __HAL_RCC_GPIOB_CLK_ENABLE();} while (0)


//定义KEYY按键的类型为GPIOE
#define     KEY_Y_GPIO_PORT              GPIOB

//定义KEYY的引脚号为0
#define     KEY_Y_GPIO_PIN               GPIO_PIN_11

//使能KEYY引脚的时钟
#define     KEY_Y_GPIO_CLK_ENABLE()        do { __HAL_RCC_GPIOB_CLK_ENABLE();} while (0)



//定义KEYG按键的类型为GPIOE
#define     KEY_G_GPIO_PORT              GPIOB

//定义KEYG的引脚号为0
#define     KEY_G_GPIO_PIN               GPIO_PIN_12

//使能KEYG引脚的时钟
#define     KEY_G_GPIO_CLK_ENABLE()        do { __HAL_RCC_GPIOB_CLK_ENABLE();} while (0)

//2，读取引脚状态

//读取KEYR是否按下
#define     KEY_R_STATUS                 ((HAL_GPIO_ReadPin(KEY_R_GPIO_PORT, KEY_R_GPIO_PIN) == GPIO_PIN_SET) ? 1 : 0 )

//读取KEYY是否按下
#define     KEY_Y_STATUS                 ((HAL_GPIO_ReadPin(KEY_Y_GPIO_PORT, KEY_Y_GPIO_PIN) == GPIO_PIN_SET) ? 1: 0 )      

//读取KEYG是否按下
#define     KEY_G_STATUS                 ((HAL_GPIO_ReadPin(KEY_G_GPIO_PORT, KEY_G_GPIO_PIN) == GPIO_PIN_SET) ? 1: 0 )

//3，键值定义
//没有任何按键按下
#define     NONE_PRESS                  0

//按下KEYR
#define     KEY_R_PRESS                  1

//按下KEYY
#define     KEY_Y_PRESS                  2

//按下KEYG
#define     KEY_G_PRESS                  3


//4，函数声明
//初始化按键
void key_e_init(void);

//扫描按键
uint8_t key_e_scan(uint8_t mode);


#endif

