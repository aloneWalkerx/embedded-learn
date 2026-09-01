#include "./BSP/BUZZER/buzzer.h"


/**
 * @brief 手动初始化蜂鸣器引脚
 */
void buzzer_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. 开启 GPIOF 端口的时钟 (关键！不打开时钟，写寄存器无效)
    __HAL_RCC_GPIOF_CLK_ENABLE();

    // 2. 配置引脚参数
    GPIO_InitStruct.Pin = BUZZER_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   // 推挽输出
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;           // 下啦
    
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  // 低速
    
    // 3. 执行初始化
    HAL_GPIO_Init(BUZZER_GPIO_PORT, &GPIO_InitStruct);

    // 4. 默认关闭蜂鸣器 (输出高电平)
   // HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_SET);
    
    buzzer_switch(1);
}



