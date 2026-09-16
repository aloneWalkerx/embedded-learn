/* ============================================================================
 * led.c —— LED 驱动实现（BSP 层）
 * ----------------------------------------------------------------------------
 * 分层归属：BSP（板级支持包）
 *   BSP 层只干一件事：把硬件操作封装成函数，供上层调用
 *   上层（app）不需要知道 LED 在哪个引脚、怎么配寄存器
 * ==========================================================================*/

/* 包含 sys.h：提供系统类型定义（uint32_t 等）和系统函数声明
 * ⚠️ 用 "./" 相对路径是正点原子教程风格；现代做法是配好 Include Paths 直接 #include "sys.h" */
#include "./SYSTEM/SYS/sys.h"

/* 包含本模块头文件：拿到 LED 的宏定义和 led_init 的声明
 * 包含自己的头文件是好习惯——可以让编译器检查"声明与定义是否一致" */
#include "./BSP/LED/led.h"


/* ----------------------------------------------------------------------------
 * led_init —— LED 初始化
 * 调用时机：main() 里 HAL_Init() → 时钟配置 → delay_init() 之后
 * 做的事：① 开 GPIOF 时钟  ② 配 PF9/PF10 为推挽输出  ③ 默认熄灭
 * --------------------------------------------------------------------------*/
void led_init(void)
{
 /* ① 开 LED0（GPIOF）的时钟
  * 展开：SET_BIT(RCC->AHB1ENR, 1<<5) + 回读插延迟
  * 【必须第一步】不开时钟，下面所有寄存器写入都无效，而且不报错 */
 LED0_GPIO_CLK_ENABLE();

 /* ② 开 LED1 的时钟
  * 注意：LED1 也在 GPIOF，这行实际是重复的
  * 重复调用无害（SET_BIT 幂等），保留是为了将来 LED 分到不同端口时可扩展 */
 LED1_GPIO_CLK_ENABLE();
 
 /* ③ 声明 HAL 的 GPIO 初始化结构体，并【清零】
  * {0} 很重要：不清零的话，未赋值的成员是栈上的随机值
  * 比如 Alternate 字段如果乱，复用模式会配错 */
 GPIO_InitTypeDef gpio_init_struct = {0};
 
 /* ④ 指定要配置的引脚：PF9 | PF10（按位或，一次配两个）
  * ⚠️ HAL 要求同一端口（GPIOF）的引脚才能一起传
  *    如果两个 LED 在不同端口，必须分开调用两次 HAL_GPIO_Init */
 gpio_init_struct.Pin = LED0_GPIO_PIN | LED1_GPIO_PIN;
 
 /* ⑤ 模式：推挽输出（PP = Push-Pull）
  * 这个字段在 HAL 内部被【拆成两半】用：
  *   - MODER  寄存器（方向：输出）  ← (Mode & GPIO_MODE)
  *   - OTYPER 寄存器（类型：推挽）  ← (Mode & OUTPUT_TYPE)
  * 推挽 = 内部上下两个 MOS 管交替导通，能【主动】输出高/低电平
  * 对比：GPIO_MODE_OUTPUT_OD = 开漏，只能拉低（I2C 用那种） */
 gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
 
 /* ⑥ 上下拉：NOPULL（无上拉无下拉）
  * 【为什么输出模式用 NOPULL】
  *   推挽输出由内部驱动器主动驱动，上下拉对【稳态电平】没有影响
  *   上下拉只在两种场合有意义：
  *     · 输入模式（浮空时给个确定默认电平）
  *     · 开漏输出（高电平靠外部上拉）
  * ✅ 这行原来是 GPIO_PULLDOWN，经过讨论后修正为 GPIO_NOPULL */
 gpio_init_struct.Pull = GPIO_NOPULL;
 
 /* ⑦ 输出速度：LOW（最低档）
  * 档位：LOW / MEDIUM / HIGH / VERY_HIGH
  * 速度越高 → 边沿越陡、EMI 越大、功耗略增
  * 点灯这种慢速应用，LOW 足够 —— 【够用就别调高】 */
 gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
 
 /* ⑧ 调用 HAL 初始化
  * 内部实际做的事（源码 stm32f4xx_hal_gpio.c:168~235）：
  *   OSPEEDR ← Speed              (行 194~197)
  *   OTYPER  ← Mode 的输出类型位   (行 200~203)
  *   PUPDR   ← Pull               (行 212~215)
  *   MODER   ← Mode 的方向位       (行 231~234)
  * ⚠️ 注意顺序：MODER 写在【最后】
  *    因为 MODER 决定引脚方向，放在最后 → 引脚直到最后一步才真正变成输出
  *    前面配 PUPDR/OTYPER/OSPEEDR 时引脚还是输入态，避免输出错误电平 */
 HAL_GPIO_Init(LED0_GPIO_PIN_TYPE, &gpio_init_struct);

 /* ⑨ 初始化后先熄灭
  * LED 是低电平点亮，所以 OFF = 输出高
  * 好习惯：避免上电瞬间 LED 乱亮 */
 LED0_OFF();
 LED1_OFF();

}
