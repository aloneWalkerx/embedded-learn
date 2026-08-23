#ifndef __LED_I_H
#define __LED_I_H


#include "./SYSTEM/sys/sys.h"

//1，板载LED0引脚定义：

//LED0引脚类型定义：F型引脚
#define     LED0_GPIO_PORT              GPIOF

//LED0引脚号定义：9号引脚
#define     LED0_GPIO_PIN               GPIO_PIN_9

//使能LED0对应引脚类型的时钟
#define     LED0_GPIO_CLK_ENABLE()      do { __HAL_RCC_GPIOF_CLK_ENABLE();} while (0)

//2，板载LED1引脚定义：

//LED1引脚类型定义：F型引脚
#define     LED1_GPIO_PORT              GPIOF

//LED1引脚号定义：10号引脚
#define     LED1_GPIO_PIN               GPIO_PIN_10

//使能LED1对应引脚类型的时钟
#define     LED1_GPIO_CLK_ENABLE()        do { __HAL_RCC_GPIOF_CLK_ENABLE(); } while (0)

//3，对板载的LED0引脚进行写操作（x为1则赋值为1，x不为1则赋值为0）
#define     LED0(x)                     do { (x) ?                                                                  \
                                            HAL_GPIO_WritePin(LED0_GPIO_PORT, LED0_GPIO_PIN, GPIO_PIN_SET):         \
                                            HAL_GPIO_WritePin(LED0_GPIO_PORT, LED0_GPIO_PIN, GPIO_PIN_RESET);       \
                                           } while (0)
                                            

//4，对板载的LED1引脚进行写操作（x为1则赋值为1，x不为1则赋值为0）                                           
#define     LED1(x)                      do { (x) ? \
                                            HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_SET):\
                                            HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_RESET);\
                                           } while (0)                     


//5，对板载的LED0引脚进行状态切换
#define     LED0_TOGGLE()                do { HAL_GPIO_TogglePin(LED0_GPIO_PORT, LED0_GPIO_PIN); } while (0)

//6，对板载的LED1引脚进行状态切换
#define     LED1_TOGGLE()                do { HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_GPIO_PIN); } while (0)

//7，LED板载初始化函数声明
void led_i_init(void);

#endif
