/* ============================================================================
 * exti.h —— 外部中断（EXTI）按键驱动头文件
 * ----------------------------------------------------------------------------
 * 提供：GPIO 引脚宏、时钟使能宏、中断号宏、ISR 名字宏、标志位声明、函数声明
 * 硬件：KEYUP → PA0（EXTI0），KEY0 → PE4（EXTI4）
 *       两键公共端接 VCC3.3 → 按下为高电平 → 内部下拉
 * ==========================================================================*/

#ifndef __EXTI_H                              /* 头文件保护：防止同一个文件被重复包含 */
#define __EXTI_H                              /* 定义保护宏：第二次包含时 __EXTI_H 已存在，直接跳到 #endif */


/* 包含 sys.h：提供 uint8_t 类型、HAL 库入口、GPIO_TypeDef、IRQn_Type 等基础定义 */
#include "./SYSTEM/SYS/sys.h"


/* ============================================================================
 * KEYUP（WK_UP）按键的硬件抽象
 * ----------------------------------------------------------------------------
 * 用途：业务层只写 KEYUP_EXTI_xxx，不关心它挂在 GPIOA 的哪个引脚
 * 好处：换板子只改这一组宏，exti.c / main.c 不用动
 * ==========================================================================*/

#define KEYUP_EXTI_PIN_TYPE             GPIOA                    /* KEYUP 所在端口：GPIOA */
#define KEYUP_EXTI_PIN                  GPIO_PIN_0               /* KEYUP 所在引脚：PA0（对应 WK_UP 功能）*/

#define KEYUP_EXTI_CLK_ENABLE()         do{ __HAL_RCC_GPIOA_CLK_ENABLE();}while(0)
                                                                 /* 开 GPIOA 时钟的"语句宏"
                                                                  * 用 do{}while(0) 包起来：
                                                                  *   保证在 if/else 里当单条语句用，不会多出分号导致断句错误
                                                                  * 末尾不加分号：由调用者写 `KEYUP_EXTI_CLK_ENABLE();`
                                                                  * 内部调 HAL 宏：置位 RCC->AHB1ENR 的 bit0（GPIOAEN）*/

#define KEYUP_EXTI_IRQn                 EXTI0_IRQn               /* 中断号：EXTI0 对应的 IRQn
                                                                  * 传给 HAL_NVIC_EnableIRQ() / SetPriority()
                                                                  * 值来自 stm32f407xx.h 的 IRQn_Type 枚举
                                                                  * ?? PA0 只能映射到 EXTI0，所以固定是 EXTI0_IRQn */

#define KEYUP_EXTI_IRQHandler           EXTI0_IRQHandler         /* ISR 名字别名
                                                                  * exti.c 里写 `void KEYUP_EXTI_IRQHandler(void)`，
                                                                  * 预处理后变成 `void EXTI0_IRQHandler(void)`，
                                                                  * 正好覆盖启动文件向量表里的弱符号
                                                                  * ?? 名字必须和向量表完全一致，拼错中断永远不响应 */


/* ============================================================================
 * KEY0 按键的硬件抽象
 * ----------------------------------------------------------------------------
 * ?? 硬件特殊点：KEY0（PE4）与 BOOT0 共用！
 *    上电/复位瞬间按住 KEY0 → BOOT0 被拉高 → MCU 进入 Bootloader 模式
 *    上电后 PE4 正常作为普通输入使用，不影响
 * ==========================================================================*/

#define KEY0_EXTI_PIN_TYPE              GPIOE                    /* KEY0 所在端口：GPIOE */
#define KEY0_EXTI_PIN                   GPIO_PIN_4               /* KEY0 所在引脚：PE4 */

#define KEY0_EXTI_CLK_ENABLE()          do{ __HAL_RCC_GPIOE_CLK_ENABLE();}while(0)
                                                                 /* 开 GPIOE 时钟
                                                                  * 置位 RCC->AHB1ENR 的 bit4（GPIOEEN）*/

#define KEY0_EXTI_IRQn                  EXTI4_IRQn               /* 中断号：EXTI4
                                                                  * ?? 和 KEYUP 不一样：PA0→EXTI0，PE4→EXTI4
                                                                  *   引脚号的低 4 位决定 EXTI 线号：4 → EXTI4 */

#define KEY0_EXTI_IRQHandler            EXTI4_IRQHandler         /* ISR 名字别名
                                                                  * exti.c 里的 KEY0_EXTI_IRQHandler 展开后是 EXTI4_IRQHandler
                                                                  * ?? 曾经写成 EXTI4_IRQHanlder（Handler 拼错）→ 中断完全没响应
                                                                  *   而且会掉进启动文件的默认死循环，把整颗芯片卡住 */


/* ============================================================================
 * 全局标志位声明（跨文件共享）
 * ----------------------------------------------------------------------------
 * 定义在 exti.c，其他文件（如 main.c）通过 extern 引用
 * 作用：ISR 与主循环之间的通信桥梁
 *   中断发生时：ISR 里写 1
 *   主循环处理时：读值 → 处理 → 写 0
 *
 * ?? 必须是 volatile：
 *   ISR 里写、主循环里读，编译器看不到"谁改了它"，
 *   不加 volatile 会被优化成常量判断，永远读不到更新
 * ==========================================================================*/

extern volatile uint8_t g_keyup_flag;         /* KEYUP 中断标志位
                                                * 1 = 有中断事件待处理；0 = 无
                                                * 由 ISR 写 1，由 key_exti_process() 清 0 */

extern volatile uint8_t g_key0_flag;          /* KEY0 中断标志位，语义同上 */


/* ============================================================================
 * 对外函数声明
 * ==========================================================================*/

void exti_init(void);                          /* 外部中断初始化
                                                * 内部：开时钟 + 配 PA0/PE4 为上升沿中断 + 配 NVIC */

void key_exti_process(void);                   /* 主循环调用的按键处理函数（非阻塞消抖）
                                                * 前提：主循环必须配合 delay_ms(10) 作为时间基准
                                                * 逻辑：标志位为 1 时计数消抖，连续 N 次确认才执行业务 */


#endif                                          /* 结束头文件保护 */
