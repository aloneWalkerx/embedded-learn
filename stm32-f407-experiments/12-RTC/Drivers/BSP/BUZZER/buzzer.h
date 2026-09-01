
#ifndef __BUZZER_H
#define __BUZZER_H

#include "./SYSTEM/sys/sys.h"

// 定义引脚宏，方便修改
#define BUZZER_GPIO_PORT    GPIOF
#define BUZZER_GPIO_PIN     GPIO_PIN_2

void buzzer_GPIO_Init(void);
///**
// * @brief 蜂鸣器响
// */
//void Buzzer_On(void)
//{
//    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_RESET); // 低电平 -> 响
//}

///**
// * @brief 蜂鸣器灭
// */
//void Buzzer_Off(void)
//{
//    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_SET);   // 高电平 -> 灭
//}

//    void buzzer_GPIO_Init(void);                     
//1 = 高电平（灭） ；0 = 低电平（响）
# define buzzer_switch(x) HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#endif

