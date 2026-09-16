#ifndef  __LED_H
#define  __LED_H

/* ============================================================================
 * LED 硬件定义（来源：正点原子《M144Z-M4最小系统板IO引脚分配表》）
 * ----------------------------------------------------------------------------
 * LED0：PF9，板上红色 LED
 * LED1：PF10，板上绿色 LED
 *
 * ⚠️ 两个引脚的"是否引出"列均为 N —— 没有引到排针，只能用板载 LED
 * ⚠️ 点亮方式：低电平点亮（LED 阳极经限流电阻接 VCC，阴极接 GPIO）
 *    → 所以 ON  = 输出低（GPIO_PIN_RESET）
 *       OFF = 输出高（GPIO_PIN_SET）
 * ==========================================================================*/


/* ---------------------------------------------------------------------------
 * LED0 相关定义
 * -------------------------------------------------------------------------*/

/* LED0 所在端口：GPIOF
 * HAL 里端口用 GPIO_TypeDef * 表示，GPIOF 展开为 ((GPIO_TypeDef *) GPIOF_BASE) */
#define LED0_GPIO_PIN_TYPE					GPIOF

/* LED0 引脚号：第 9 号
 * GPIO_PIN_9 是 HAL 的位掩码写法，实际值是 (1UL << 9) = 0x0200 */
#define LED0_GPIO_PIN						GPIO_PIN_9

/* LED0 的 GPIO 时钟使能
 * 展开后（见 stm32f4xx_hal_rcc_ex.h:2079）：
 *     SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOFEN);   // 置位 AHB1ENR 第 5 位
 *     tmpreg = READ_BIT(RCC->AHB1ENR, ...);         // 回读，插入延迟
 *
 * 【为什么必须第一步开时钟】
 *   不开时钟 → GPIO 寄存器写入无效 → 引脚没反应
 *   而且【不报错】：编译通过、下载成功、代码看着全对，就是灯不亮
 *   原因：STM32 是门控时钟设计，不给外设供时钟，外设逻辑根本不工作
 *
 * 【为什么末尾要回读一次】
 *   使能时钟后外设寄存器不是立刻可访问，需要几个时钟周期
 *   用读一个 volatile 变量来插入延迟——这样编译器优化不掉
 *
 * 【为什么用 do{}while(0) 包起来】
 *   保证宏在 if/else 里当单条语句使用时不会出错（不会多出分号断句） */
#define LED0_GPIO_CLK_ENABLE()				do{ __HAL_RCC_GPIOF_CLK_ENABLE();}while(0)


/* ---------------------------------------------------------------------------
 * LED1 相关定义
 * -------------------------------------------------------------------------*/

/* LED1 所在端口：同样是 GPIOF */
#define LED1_GPIO_PIN_TYPE					GPIOF

/* LED1 引脚号：第 10 号 */
#define LED1_GPIO_PIN  						GPIO_PIN_10

/* LED1 的 GPIO 时钟使能
 * ⚠️ 和 LED0 展开后是同一个宏（都在 GPIOF），这句是冗余的
 *    但重复调用无害（SET_BIT 幂等，置 1 多次结果一样）
 *    保留这种写法是为了"将来 LED 分到不同端口"时可扩展 —— 好习惯 */
#define LED1_GPIO_CLK_ENABLE()				do{ __HAL_RCC_GPIOF_CLK_ENABLE();}while(0)


/* ---------------------------------------------------------------------------
 * LED 操作宏
 * -------------------------------------------------------------------------*/

/* LED0 点亮：引脚输出【低】电平
 * HAL_GPIO_WritePin 内部用 BSRR 的【高 16 位】（复位位，写 1 有效）
 * 见 stm32f4xx_hal_gpio.c:422 → GPIOx->BSRR = (uint32_t)GPIO_Pin << 16U; */
#define LED0_ON()							do{ HAL_GPIO_WritePin(LED0_GPIO_PIN_TYPE, LED0_GPIO_PIN, GPIO_PIN_RESET);}while(0)

/* LED0 熄灭：引脚输出【高】电平
 * 内部用 BSRR 的【低 16 位】（置位位，写 1 有效）
 * 见 stm32f4xx_hal_gpio.c:418 → GPIOx->BSRR = GPIO_Pin; */
#define LED0_OFF()							do{ HAL_GPIO_WritePin(LED0_GPIO_PIN_TYPE, LED0_GPIO_PIN, GPIO_PIN_SET);}while(0)

/* LED1 点亮：输出低 */
#define LED1_ON()							do{ HAL_GPIO_WritePin(LED1_GPIO_PIN_TYPE, LED1_GPIO_PIN, GPIO_PIN_RESET);}while(0)

/* LED1 熄灭：输出高 */
#define LED1_OFF()							do{ HAL_GPIO_WritePin(LED1_GPIO_PIN_TYPE, LED1_GPIO_PIN, GPIO_PIN_SET);}while(0)


/* 对外暴露的初始化函数（实现在 led.c） */
void led_init(void);


#endif
