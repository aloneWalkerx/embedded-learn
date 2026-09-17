/* ============================================================================
 * exti.c —— 外部中断（EXTI）按键驱动
 * ----------------------------------------------------------------------------
 * 硬件：KEYUP → PA0（EXTI0），KEY0 → PE4（EXTI4）
 *       两键公共端接 VCC3.3 → 按下为高电平 → 内部下拉
 * 架构：中断触发（ISR 只置标志）+ 主循环消抖处理（非阻塞）
 * ==========================================================================*/

#include "./SYSTEM/SYS/sys.h"                                   /* HAL 库 + 基础类型 */
#include "./BSP/EXTI/exti.h"                                    /* 本模块宏与声明 */
#include "./BSP/LED/led.h"                                      /* LED0_TOGGLE / LED1_TOGGLE */


/* ============================================================================
 * 全局变量：ISR 与主循环之间的通信桥梁
 * ==========================================================================*/

volatile uint8_t g_keyup_flag = 0;                              /* KEYUP 中断标志位
                                                                  * ISR 写 1，主循环读+清 0
                                                                  * ?? 必须 volatile：
                                                                  *   ISR 里写、主循环里读，编译器看不到"谁改了它"，
                                                                  *   不加 volatile 会被优化成常量判断，永远读不到更新 */

volatile uint8_t g_key0_flag  = 0;                              /* KEY0 中断标志位，同上 */

uint8_t g_keyup_cnt = 0;                                        /* KEYUP 消抖计数器
                                                                  * 只在 key_exti_process()（主循环）里读写
                                                                  * ?? 不需要 volatile：ISR 完全不碰它 */

uint8_t g_key0_cnt  = 0;                                        /* KEY0 消抖计数器，同上 */


/* ============================================================================
 * 消抖阈值：连续 N 次扫描到"按下"才确认
 * ----------------------------------------------------------------------------
 * 实际消抖时间 = 主循环周期 × DEBOUNCE_CNT
 *   主循环 10ms × 2 = 20ms  ← 与机械按键抖动窗口（5~20ms）匹配
 * ?? 如果主循环没加 delay，扫描周期是微秒级，这个阈值形同虚设
 * ==========================================================================*/
#define DEBOUNCE_CNT    2


/* ============================================================================
 * exti_init —— 外部中断初始化
 * ----------------------------------------------------------------------------
 * 步骤：① 开 GPIO 时钟 ② 配 PA0/PE4 为上升沿中断 ③ 配 NVIC 优先级并放行
 * ==========================================================================*/
void exti_init(void)
{
    KEYUP_EXTI_CLK_ENABLE();                                    /* ① 开 GPIOA 时钟（PA0 属于 GPIOA）
                                                                  * ?? 不开时钟 → EXTI 配置全部无效且不报错 */
    KEY0_EXTI_CLK_ENABLE();                                     /* ① 开 GPIOE 时钟（PE4 属于 GPIOE）*/

    GPIO_InitTypeDef gpio_init_struct = {0};                    /* HAL 初始化结构体，{0} = 全字段清零
                                                                  * ?? 不清零 → 未赋值成员是栈上随机值，会被写进寄存器 */

    /* ------------------ 配置 KEYUP（PA0）为上升沿中断 ------------------ */
    gpio_init_struct.Pin   = KEYUP_EXTI_PIN;                    /* 目标引脚：PA0 */
    gpio_init_struct.Mode  = GPIO_MODE_IT_RISING;               /* 中断模式 + 上升沿触发
                                                                  * 硬件上按键接 VCC，按下瞬间电平 0→1，就是上升沿 */
    gpio_init_struct.Pull  = GPIO_PULLDOWN;                     /* 下拉：松开时引脚悬空，靠下拉拉到 0，避免噪声误触发 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;               /* 输入模式下 Speed 无实际作用，HAL 照写 */

    HAL_GPIO_Init(KEYUP_EXTI_PIN_TYPE, &gpio_init_struct);      /* HAL 写寄存器：
                                                                  *   SYSCFG->EXTICR  → 把 PA0 映射到 EXTI0 线
                                                                  *   EXTI->IMR       → 放行 EXTI0 中断
                                                                  *   EXTI->RTSR      → 允许上升沿触发
                                                                  *   GPIOA->MODER/PUPDR → 复用模式 + 下拉 */

    /* ------------------ 配置 KEY0（PE4）为上升沿中断 ------------------ */
    gpio_init_struct.Pin   = KEY0_EXTI_PIN;                     /* 复用结构体，改 Pin 为 PE4 */
    gpio_init_struct.Mode  = GPIO_MODE_IT_RISING;               /* 同样是上升沿中断 */
    gpio_init_struct.Pull  = GPIO_PULLDOWN;                     /* 同样是下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;               /* 同上，输入模式下无实际作用 */

    HAL_GPIO_Init(KEY0_EXTI_PIN_TYPE, &gpio_init_struct);       /* 配 GPIOE 的对应寄存器，映射到 EXTI4 线 */

    /* ------------------ 配置 NVIC ------------------ */
    HAL_NVIC_SetPriority(KEYUP_EXTI_IRQn, 0, 0);                /* 设置 EXTI0 中断优先级
                                                                  * 参数：中断号、抢占优先级=0、子优先级=0
                                                                  * ?? 抢占优先级 0 是最高的，一般不建议按键占最高级 */
    HAL_NVIC_EnableIRQ(KEYUP_EXTI_IRQn);                        /* 允许 NVIC 向 CPU 抛出 EXTI0 中断
                                                                  * 对应寄存器：NVIC->ISER[0] 的对应位 */

    HAL_NVIC_SetPriority(KEY0_EXTI_IRQn, 0, 0);                 /* EXTI4 优先级，同上 */
    HAL_NVIC_EnableIRQ(KEY0_EXTI_IRQn);                         /* 允许 EXTI4 中断 */
}


/* ============================================================================
 * 中断服务函数（ISR）
 * ----------------------------------------------------------------------------
 * ?? 函数名由启动文件 startup_stm32f40_41xxx.s 的向量表定死，拼错中断就不响应
 * ?? 名称必须和 exti.h 里的宏 KEYUP_EXTI_IRQHandler 对应
 * ==========================================================================*/
void KEYUP_EXTI_IRQHandler(void)                                /* 宏展开 = EXTI0_IRQHandler */
{
    HAL_GPIO_EXTI_IRQHandler(KEYUP_EXTI_PIN);                   /* HAL 通用处理：
                                                                  *   ① 读 EXTI->PR 判断是不是 PA0 触发的
                                                                  *   ② 写 EXTI->PR 对应位（写 1 清零），清挂起标志
                                                                  *   ③ 调用 HAL_GPIO_EXTI_Callback(PA0) */
}

void KEY0_EXTI_IRQHandler(void)                                 /* 宏展开 = EXTI4_IRQHandler */
{
    HAL_GPIO_EXTI_IRQHandler(KEY0_EXTI_PIN);                    /* 同上，处理 EXTI4 通道 */
}


/* ============================================================================
 * HAL 弱回调函数 —— 用户业务逻辑入口
 * ----------------------------------------------------------------------------
 * 原则：ISR 里只做最短的事（置标志位），耗时逻辑放主循环
 * ?? 绝不能在这里 delay_ms() / printf() / 写 Flash / 跑长循环
 * ==========================================================================*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (KEYUP_EXTI_PIN == GPIO_Pin)                             /* 判断是不是 KEYUP（PA0）触发的
                                                                  * 注意：HAL_GPIO_ReadPin 和这里判断的是不同东西
                                                                  *   这里判断的是"哪条 EXTI 线"，不是"当前电平" */
    {
        g_keyup_flag = 1;                                       /* 只置一个标志位，立即返回
                                                                  * 抖动导致的多次触发 → 标志位被写多次，值都是 1
                                                                  * 主循环处理完清 0 即可，不会累加、不会丢事件 */
    }

    if (KEY0_EXTI_PIN == GPIO_Pin)                              /* 判断是不是 KEY0（PE4）触发的 */
    {
        g_key0_flag = 1;                                        /* 同上，只置标志位 */
    }
}


/* ============================================================================
 * key_exti_process —— 主循环调用的按键处理函数（非阻塞消抖）
 * ----------------------------------------------------------------------------
 * 调用时机：主循环里，配合 delay_ms(10) 作为时间基准
 * 逻辑：标志位为 1 时开始消抖计数 → 连续 N 次读到按下 → 执行业务
 * ==========================================================================*/
void key_exti_process(void)
{
    /* ================= 处理 KEYUP（PA0） ================= */
    if (g_keyup_flag == 1)                                      /* 有中断事件待处理（ISR 置的）*/
    {
        if (HAL_GPIO_ReadPin(KEYUP_EXTI_PIN_TYPE, KEYUP_EXTI_PIN) == GPIO_PIN_SET)
                                                                /* 当前电平还是"按下"状态 */
        {
            if (++g_keyup_cnt >= DEBOUNCE_CNT)                  /* 先自增再比较：连续 DEBOUNCE_CNT 次都按下
                                                                  * 用 ++ 前缀先加再用 */
            {
                LED0_TOGGLE();                                  /* ? 确认成功 → 执行业务 */
                g_keyup_cnt  = 0;                               /* 计数清零，为下次做准备 */
                g_keyup_flag = 0;                               /* 清标志位，本次事件处理完 */
            }
            /* 未达阈值 → 什么都不做，保留 flag=1，下轮继续扫 */
        }
        else                                                    /* 中途读到"松开" → 本次是抖动误触发 */
        {
            g_keyup_cnt  = 0;                                   /* 计数清零 */
            g_keyup_flag = 0;                                   /* 清标志，本次放弃 */
        }
    }

    /* ================= 处理 KEY0（PE4） ================= */
    if (g_key0_flag == 1)                                       /* 同上逻辑，处理 KEY0 */
    {
        if (HAL_GPIO_ReadPin(KEY0_EXTI_PIN_TYPE, KEY0_EXTI_PIN) == GPIO_PIN_SET)
        {
            if (++g_key0_cnt >= DEBOUNCE_CNT)
            {
                LED1_TOGGLE();                                  /* ? 确认成功 → 翻转 LED1 */
                g_key0_cnt  = 0;
                g_key0_flag = 0;
            }
        }
        else
        {
            g_key0_cnt  = 0;
            g_key0_flag = 0;
        }
    }
}
