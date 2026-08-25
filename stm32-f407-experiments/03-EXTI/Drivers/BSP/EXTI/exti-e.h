#ifndef EXTI_E_H
#define EXTI_E_H


//1，外设按键控制外设红灯
//引脚类型
#define  KEY_R_EXT_GPIO_PORT            GPIOB

//引脚号
#define  KEY_R_EXT_GPIO_PIN             GPIO_PIN_10

//使能引脚类型
#define  KEY_R_EXT_GPIO_CLK_ENABLE()   do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while (0)

//定义对应中断号
#define  KEY_R_EXT_IRQn                 EXTI15_10_IRQn

//定义对应中断服务函数
#define  KEY_R_EXT_IRQHandler           EXTI15_10_IRQHandler



//2，外设按键控制外设黄灯
//引脚类型
#define  KEY_Y_EXT_GPIO_PORT            GPIOB

//引脚号
#define  KEY_Y_EXT_GPIO_PIN             GPIO_PIN_11

//使能引脚类型
#define  KEY_Y_EXT_GPIO_CLK_ENABLE()   do { __HAL_RCC_GPIOB_CLK_ENABLE();} while (0)

//定义对应中断号
#define  KEY_Y_EXT_IRQn                 EXTI15_10_IRQn

//定义对应中断服务函数
#define  KEY_Y_EXT_IRQHandler           EXTI15_10_IRQHandler


//3，外设按键控制外设绿灯
//引脚类型
#define  KEY_G_EXT_GPIO_PORT            GPIOB

//引脚号
#define  KEY_G_EXT_GPIO_PIN             GPIO_PIN_12

//使能引脚类型
#define  KEY_G_EXT_GPIO_CLK_ENABLE()   do { __HAL_RCC_GPIOB_CLK_ENABLE();} while (0)

//定义对应中断号
#define  KEY_G_EXT_IRQn                 EXTI15_10_IRQn

//定义对应中断服务函数
#define  KEY_G_EXT_IRQHandler           EXTI15_10_IRQHandler


//4，初始化外设按键控制外设LED中断
void exti_e_init(void);

#endif

