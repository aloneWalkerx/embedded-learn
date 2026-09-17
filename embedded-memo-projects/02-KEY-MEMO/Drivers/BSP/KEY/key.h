#ifndef __KEY_H
#define __KEY_H

#include "./SYSTEM/SYS/sys.h"

#define KEYUP_GPIO_PIN_TYPE                 GPIOA
#define KEYUP_GPIO_PIN                      GPIO_PIN_0
#define KEYUP_GPIO_CLK_ENABLE()             do{ __HAL_RCC_GPIOA_CLK_ENABLE();}while(0)

#define KEY0_GPIO_PIN_TYPE                  GPIOE
#define KEY0_GPIO_PIN                       GPIO_PIN_4
#define KEY0_GPIO_CLK_ENABLE()              do{ __HAL_RCC_GPIOE_CLK_ENABLE();}while(0)


#define READ_KEYUP_PIN                      ((HAL_GPIO_ReadPin(KEYUP_GPIO_PIN_TYPE, KEYUP_GPIO_PIN) == GPIO_PIN_SET) ? 1 : 0)
#define READ_KEY0_PIN                       ((HAL_GPIO_ReadPin(KEY0_GPIO_PIN_TYPE, KEY0_GPIO_PIN) == GPIO_PIN_SET) ? 1 : 0)

#define NONE_PRESS                          0
#define KEYUP_PRESS                         1
#define KEY0_PRESS                          2


void key_init(void);
uint8_t key_scan(uint8_t keyMode);



#endif

