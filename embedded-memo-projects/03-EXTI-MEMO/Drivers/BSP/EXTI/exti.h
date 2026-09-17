#ifndef __EXTI_H
#define __EXTI_H

#define KEYUP_EXTI_PIN_TYPE                 GPIOA
#define KEYUP_EXTI_PIN                      GPIO_PIN_0
#define KEYUP_EXTI_CLK_ENABLE()             do{ __HAL_RCC_GPIOA_CLK_ENABLE();}while(0)
#define KEYUP_EXTI_IRQn                     EXTI0_IRQn
#define KEYUP_EXTI_IRQHandler               EXTI0_IRQHandler


#define KEY0_EXTI_PIN_TYPE                  GPIOE
#define KEY0_EXTI_PIN                       GPIO_PIN_4
#define KEY0_EXTI_CLK_ENABLE()              do{ __HAL_RCC_GPIOE_CLK_ENABLE();}while(0)
#define KEY0_EXTI_IRQn                      EXTI4_IRQn
#define KEY0_EXTI_IRQHandler                EXTI4_IRQHandler




void exti_init(void);

#endif