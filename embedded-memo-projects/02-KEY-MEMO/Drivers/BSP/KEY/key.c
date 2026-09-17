/* ============================================================================
 * key.c —— 按键板级驱动实现
 * ----------------------------------------------------------------------------
 * 依赖：sys.h（HAL 入口）、key.h（本模块头文件）、delay.h（消抖延时）
 * 实现：GPIO 输入初始化 + 状态机扫描 + 延时消抖
 * ==========================================================================*/

#include "./SYSTEM/SYS/sys.h"                                    /* HAL 库 + 基础类型 */
#include "./BSP/KEY/key.h"                                       /* 本模块宏与声明 */
#include "./SYSTEM/DELAY/delay.h"                                /* delay_ms() 用于消抖 */


/* ============================================================================
 * key_init —— 按键 GPIO 初始化
 * ----------------------------------------------------------------------------
 * 步骤：① 开时钟 ② 配 PA0 为下拉输入 ③ 配 PE4 为下拉输入
 * 说明：输入模式下 Speed/OTYPER 硬件不生效，HAL 照写但无副作用
 * ==========================================================================*/
void key_init(void)
{
    KEYUP_GPIO_CLK_ENABLE();                                     /* ① 开 GPIOA 时钟（PA0 属于 GPIOA）
                                                                  * 【必须第一步】不开时钟，后面写 MODER/PUPDR 全部无效，且不报错 */
    KEY0_GPIO_CLK_ENABLE();                                      /* ① 开 GPIOE 时钟（PE4 属于 GPIOE）*/

    GPIO_InitTypeDef gpio_init_struct = {0};                     /* 声明 HAL 初始化结构体
                                                                  * {0} = 全字段清零
                                                                  * ?? 不清零的话，未赋值成员是栈上随机值，会被 HAL 一起写进寄存器 */

    /* ------------------ 配置 KEYUP（PA0） ------------------ */
    gpio_init_struct.Pin   = KEYUP_GPIO_PIN;                     /* 目标引脚：PA0 */
    gpio_init_struct.Mode  = GPIO_MODE_INPUT;                    /* 输入模式（MODER = 00）*/
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;                /* 输入模式下此字段硬件不生效，HAL 照写 */
    gpio_init_struct.Pull  = GPIO_PULLDOWN;                      /* 下拉（PUPDR = 10）
                                                                  * 为什么下拉：按键公共端接 VCC3.3
                                                                  *   按下 → 引脚被 3.3V 拉高 → 读到 1
                                                                  *   松开 → 引脚悬空，靠下拉拉到 GND → 读到 0 */
    HAL_GPIO_Init(KEYUP_GPIO_PIN_TYPE, &gpio_init_struct);       /* HAL 写寄存器：配 GPIOA 的 MODER/PUPDR/OSPEEDR/OTYPER
                                                                  * 源码位置：stm32f4xx_hal_gpio.c:180 附近的 HAL_GPIO_Init */

    /* ------------------ 配置 KEY0（PE4） ------------------ */
    gpio_init_struct.Pin   = KEY0_GPIO_PIN;                      /* 复用同一结构体，改 Pin 为 PE4
                                                                  * Mode/Speed/Pull 沿用上一段，值未变 → 不重复赋值 */
    gpio_init_struct.Mode  = GPIO_MODE_INPUT;                    /* 显式重写一次，增强可读性（其实和上一段相同）*/
    gpio_init_struct.Pull  = GPIO_PULLDOWN;                      /* 同样是下拉（按键公共端也接 VCC3.3）*/
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;                /* 同上，输入模式下无实际作用 */

    HAL_GPIO_Init(KEY0_GPIO_PIN_TYPE, &gpio_init_struct);        /* HAL 写 GPIOE 的 MODER/PUPDR
                                                                  * ?? 端口从 GPIOA 换成 GPIOE，别写成同一个 */
}


/* ============================================================================
 * key_scan —— 扫描一次按键
 * ----------------------------------------------------------------------------
 * 参数：keyMode
 *         0 → 不支持连按（按下→松开→再按下 才能触发第二次）
 *         非0 → 支持连按（每调用一次就会重新判定，配合主循环相当于按住持续触发）
 * 返回：NONE_PRESS / KEYUP_PRESS / KEY0_PRESS
 *
 * 状态机：RELEASED（可触发）←→ PRESSED（锁定中）
 *   - key_release_status == 1 → 处于 RELEASED，允许触发
 *   - key_release_status == 0 → 处于 PRESSED，锁定，防止一次按下多次触发
 * ==========================================================================*/
uint8_t key_scan(uint8_t keyMode)
{
    static uint8_t key_release_status = 1;                       /* 静态变量：跨调用保持状态
                                                                  * 初值 1 → 上电时认为"已释放"，允许第一次触发
                                                                  * ?? 必须加 static！普通局部变量每次进函数都会重置为 1，状态机失效 */
    uint8_t key_value = NONE_PRESS;                              /* 本次扫描的返回值，默认为"无按键" */

    /* ------------------ 处理连按模式 ------------------ */
    if (keyMode != 0)                                            /* 如果调用者要求支持连按 */
    {
        key_release_status = 1;                                  /* 强制解锁 → 下次判断必然允许触发
                                                                  * 效果：只要按键还按着，每次调用都会返回一次键值 */
    }

    /* ------------------ 主判断：检测按下 ------------------ */
    if ((key_release_status == 1) &&                             /* 条件1：当前处于"已释放"状态（未锁定）*/
        ((READ_KEYUP_PIN == 1) || (READ_KEY0_PIN == 1)))         /* 条件2：KEYUP 或 KEY0 有任意一个被按下 */
    {
        delay_ms(10);                                            /* ? 消抖：机械按键抖动持续 5~20ms
                                                                  * 延时 10ms 跳过抖动期，再判一次 */
        key_release_status = 0;                                  /* ?? 立刻锁定：防止松手前多次进入本分支
                                                                  * 解锁条件：下面 else if 分支检测到两键全松 */

        if (READ_KEYUP_PIN == 1)                                 /* 判定具体是哪个键（先判 KEYUP，优先级高）
                                                                  * ?? 这里其实没"重读确认"！(见下面的改进提示)
                                                                  * 正确做法是：延迟后再读一次，确认仍在按下才返回 */
        {
            key_value = KEYUP_PRESS;                             /* 返回 KEYUP 键值 */
        }

        if (READ_KEY0_PIN == 1)                                  /* 用 if 而不是 else if：两键同时按下时后者覆盖前者
                                                                  * 也就是说同时按 → 最终返回 KEY0_PRESS */
        {
            key_value = KEY0_PRESS;                              /* 返回 KEY0 键值 */
        }
    }
    /* ------------------ 附加判断：检测释放 ------------------ */
    else if ((READ_KEYUP_PIN == 0) && (READ_KEY0_PIN == 0))      /* 两键都松开（都是 0）→ 说明已完全释放 */
    {
        key_release_status = 1;                                  /* 解锁 → 允许下一次按下触发
                                                                  * 用 else if：只在"上一分支没进"时才判释放，避免重复判定 */
    }

    return key_value;                                            /* 返回本次扫描结果；无按下返回 NONE_PRESS */
}

