#include "./BSP/IIC/iic.h"
#include "./SYSTEM/delay/delay.h"

I2C_HandleTypeDef i2c1def;

/**
 * @brief  I2C1 初始化（硬件 I2C，100kHz）
 */
void i2c_init(void) 
{
    // 强制复位 I2C1 外设，使其回到初始状态（关键！）
    __HAL_RCC_I2C1_FORCE_RESET();
    delay_ms(10);
    // 释放 I2C1 复位，退出复位状态
    __HAL_RCC_I2C1_RELEASE_RESET();
    delay_ms(10);

    // 指定 I2C 句柄对应的外设实例为 I2C1
    i2c1def.Instance = I2C1;

    // 设置 I2C 时钟频率为 100kHz
    i2c1def.Init.ClockSpeed = 100000;
    // 设置快速模式下的占空比（此处为 2:1）
    i2c1def.Init.DutyCycle = I2C_DUTYCYCLE_2;
    // 设置自身地址1为 0（主模式通常不用）
    i2c1def.Init.OwnAddress1 = 0;
    // 设置地址模式为 7 位地址
    i2c1def.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    // 禁用双地址模式
    i2c1def.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    // 设置自身地址2为 0（双地址禁用时忽略）
    i2c1def.Init.OwnAddress2 = 0;
    // 禁用通用呼叫地址
    i2c1def.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    // 禁用时钟拉伸（允许从机拉伸时钟）
    i2c1def.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    // 调用 HAL 库初始化 I2C1，若失败则进入死循环，等待复位
    if (HAL_I2C_Init(&i2c1def) != HAL_OK) {
        while (1);
    }

    // 使能 I2C1 外设（HAL_I2C_Init 内部通常已使能，此处确保开启）
    __HAL_I2C_ENABLE(&i2c1def);
}

/**
 * @brief  I2C1 底层引脚配置（HAL 回调）
 */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
    // 定义 GPIO 初始化结构体并清零
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // 判断是否为 I2C1 实例
    if (hi2c->Instance == I2C1) {
        // 使能 I2C1 外设时钟
        __HAL_RCC_I2C1_CLK_ENABLE();
        // 使能 GPIOB 时钟（因为 PB6/PB7 属于 GPIOB）
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /* PB6(SCL), PB7(SDA) */
        // 选择引脚 PB6 和 PB7
        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        // 设置为复用开漏模式（I2C 需要开漏）
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        // 使能内部上拉（通常外部也有上拉）
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        // 设置 GPIO 速度为高
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        // 设置复用功能为 AF4（I2C1）
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        // 根据以上参数初始化 GPIOB
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

// HAL 库 MSP 反初始化回调函数，用于复位底层硬件
void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c) 
{
    // 判断是否为 I2C1 实例
    if (hi2c->Instance == I2C1) {
        // 关闭 I2C1 外设时钟
        __HAL_RCC_I2C1_CLK_DISABLE();
        // 反初始化 PB6 和 PB7 引脚
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6 | GPIO_PIN_7); 
    }
}