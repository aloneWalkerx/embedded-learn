#ifndef  __LED_H
#define  __LED_H

#define LED0_GPIO_PIN_TYPE              GPIOF

#define LED0_GPIO_PIN                   GPIO_PIN_9

#define LED0_GPIO_CLK_ENABLE()          do { __HAL_RCC_GPIOF_CLK_ENABLE();}while(0)


#define LED1_GPIO_PIN_TYPE              GPIOF

#define LED1_GPIO_PIN                   GPIO_PIN_10

#define LED1_GPIO_CLK_ENABLE()          do { __HAL_RCC_GPIOF_CLK_ENABLE();}while(0)


#define LED0_READ_PIN_STATUS(x)         do{ (x) ? \
                                            HAL_GPIO_WritePin(LED0_GPIO_PIN_TYPE, LED0_GPIO_PIN, GPIO_PIN_SET) : \
                                            HAL_GPIO_WritePin(LED0_GPIO_PIN_TYPE, LED0_GPIO_PIN, GPIO_PIN_RESET); \
                                          }while(0)

#define LED1_READ_PIN_STATUS(x)         do{ (x) ? \
                                                HAL_GPIO_WritePin(LED1_GPIO_PIN_TYPE, LED1_GPIO_PIN, GPIO_PIN_SET) : \
                                                HAL_GPIO_WritePin(LED1_GPIO_PIN_TYPE, LED1_GPIO_PIN£¬GPIO_PIN_RESET) \
                                           }while(0)


#define LED0_ON()                       do{ HAL_GPIO_WritePin(LED0_GPIO_PIN_TYPE, LED0_GPIO_PIN, GPIO_PIN_SET);} while(0)


#define LED0_OFF()                      do { HAL_GPIO_WritePin(LED0_GPIO_PIN_TYPE, LED0_GPIO_PIN, GPIO_PIN_RESET);} while(0)

#define LED1_ON()                       do{ HAL_GPIO_WritePin(LED1_GPIO_PIN_TYPE, LED1_GPIO_PIN, GPIO_PIN_SET);}while(0)

#define LED1_OFF()                      do{ HAL_GPIO_WritePin(LED1_GPIO_PIN_TYPE, LED1_GPIO_PIN, GPIO_PIN_RESET);}while(0)


void led_init(void);

#endif 