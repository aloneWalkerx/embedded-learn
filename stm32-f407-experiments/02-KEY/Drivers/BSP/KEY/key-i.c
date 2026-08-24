#include "./BSP/KEY/key-i.h"
#include "./SYSTEM/delay/delay.h"


void key_i_init()
{   
    //1，创建引脚初始化实例
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    //2，使能引脚时钟
    //使能WKUP对应引脚类型的时钟（PA0）
    WKUP_GPIO_CLK_ENABLE();
    
    //使能KEY0对应引脚类型的时钟（PE4）
    KEY0_GPIO_CLK_ENABLE();
    
    //3，配置WKUP引脚
    //配置引脚号
    gpio_init_struct.Pin = WKUP_GPIO_PIN;
    
    //配置模式为输入模式
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    
    //配置输入模式为上拉输入
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    
    //配置速度
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    
    //根据参数初始化WKUP引脚
    HAL_GPIO_Init(WKUP_GPIO_PORT, &gpio_init_struct);
    
    
    //4，配置KEY0引脚
    //配置引脚号
    gpio_init_struct.Pin = KEY0_GPIO_PIN;
    
    //配置模式为输入模式
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    
    //配置输入模式为上拉输入
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    
    //配置速度
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    
    //根据参数初始化WKUP引脚
    HAL_GPIO_Init(KEY0_GPIO_PORT, &gpio_init_struct);
    
}


uint8_t key_i_scan(uint8_t mode)
{
    static uint8_t key_release = 1;   // 静态变量，记录上次按键是否已释放（1=已释放）
    uint8_t key_value = NONE_PRESS;   // 默认无按键

    // 【模式控制】如果 mode 不为 0（即 mode=1），强制认为按键已释放 → 开启连续触发
    if (mode != 0) {
        key_release = 1;
    }

    // 【主检测条件】只有在“已释放”状态，且任一按键被按下时，才进入处理
    if ((key_release == 1) && ((WKUP_STATUS == 1) || (KEY0_STATUS == 1))) {
        delay_ms(10);                // 软件消抖（延时约10ms）
        key_release = 0;             // 标记“已按下”，避免本次重复触发

        // 判断具体是哪个按键按下了
        if (KEY0_STATUS == 1) {
            key_value = KEY0_PRESS;
        }
        if (WKUP_STATUS == 1) {
            key_value = WKUP_PRESS;  // 如果两个同时按下，WKUP 会覆盖 KEY0（即WKUP优先于KEY0）
        }
    }
    // 【释放检测】如果两个按键都处于未按下状态（电平为0），则恢复“已释放”标志
    else if ((WKUP_STATUS == 0) && (KEY0_STATUS == 0)) {
        key_release = 1;
    }

    return key_value;   // 返回检测到的按键值（无按键则返回 NONE_PRESS）
}


