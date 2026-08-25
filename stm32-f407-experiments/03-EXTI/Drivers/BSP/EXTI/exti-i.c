#include "./BSP/EXTI/exti-i.h"

#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led-i.h"
void exti_i_init(void)
{   
    //1，创建引脚初始化实例
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    //使能WKUP对应的引脚类型（GPIOA）
    WKUP_EXT_GPIO_CLK_ENABLE();
    
    //使能KEY0对应的引脚类型（GPIOE）
    KEY0_EXT_GPIO_CLK_ENABLE();
    
    //2,配置WKUP按键对应的引脚信息
    //设置引脚号
    gpio_init_struct.Pin = WKUP_EXT_GPIO_PIN;
    
    //设置为上升沿
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING;
    
    //设置下拉
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    
    //设置速度
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    
    //根据参数初始化WKUP引脚
    HAL_GPIO_Init(WKUP_EXT_GPIO_PORT, &gpio_init_struct);
    
    //3,配置WKUP按键对应的引脚信息
    //设置引脚号
    gpio_init_struct.Pin = KEY0_EXT_GPIO_PIN;
    
    //设置为上升沿
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING;
    
    //设置下拉
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    
    //设置速度为低速
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    
    //根据参数初始化KEY0引脚
    HAL_GPIO_Init(KEY0_EXT_GPIO_PORT, &gpio_init_struct);
    
    //设置WKUP的中断优先级
    HAL_NVIC_SetPriority(WKUP_EXT_IRQn, 0, 0);
    
    //使能WKUP引脚对应的中断
    HAL_NVIC_EnableIRQ(WKUP_EXT_IRQn);
    
    //设置WKUP的中断优先级
    HAL_NVIC_SetPriority(KEY0_EXT_IRQn, 0, 0);
    
    //使能WKUP引脚对应的中断
    HAL_NVIC_EnableIRQ(KEY0_EXT_IRQn);
    
}


/*
WKUP按键外部中断服务函数

*/
void WKUP_EXT_IRQHandler()
{

    HAL_GPIO_EXTI_IRQHandler(WKUP_EXT_GPIO_PIN);

}


/*
KEY0按键外部中断服务函数

*/
void KEY0_EXT_IRQHandler()
{

    HAL_GPIO_EXTI_IRQHandler(KEY0_EXT_GPIO_PIN);
}

/*
HAL库外部中断回调函数
@param  GPIO_Pin : 外部中断线对应的引脚
*/

//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
//{

//    delay_ms(20);               /* 机械按键消抖（仅演示，切勿在实际工程的中断服务函数种进行阻塞延时） */
//    
//    switch (GPIO_Pin)
//    {
//        case WKUP_EXT_GPIO_PIN: /* WKUP按键对应引脚发生中断 */
//        {
//            LED0_TOGGLE();      /* 翻转LED0状态 */
//            break;
//        }
//        case KEY0_EXT_GPIO_PIN: /* KEY0按键对应引脚发生中断 */
//        {
//            LED1_TOGGLE();      /* 翻转LED1状态 */
//            break;
//        }
//    }
//    
//}
