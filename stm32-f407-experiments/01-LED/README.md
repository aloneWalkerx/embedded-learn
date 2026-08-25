# 实验1 :跑马灯

本实验通过STM32F407ZGT6板载的LED0（PF9）,LED1(PF10)以及外设红，黄，绿LED灯为实验对象（====末尾是实验结果视频====）

### ① 时钟树

**基于 `sys_stm32_clock_init(336, 8, 2, 7)` 的实际配置：**

text

┌─────────────────────────────────────────────────────────────────┐
│  外部晶振 HSE = 8MHz                                           │
│      │                                                         │
│      ▼                                                         │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              PLL 配置 (RCC->PLLCFGR)                    │   │
│  │  HSE/8 = 1MHz  →  ×336 = 336MHz  →  /2 = 168MHz       │   │
│  │  (pllm=8)         (plln=336)        (pllp=2)           │   │
│  └─────────────────────────────────────────────────────────┘   │
│      │                                                         │
│      ▼                                                         │
│  SYSCLK = 168MHz                                               │
│      │                                                         │
│      ├── AHB 分频 HPRE=1 → HCLK = 168MHz                      │
│      │       │                                                 │
│      │       ├── APB1 分频 PPRE1=4 → PCLK1 = 42MHz            │
│      │       └── APB2 分频 PPRE2=2 → PCLK2 = 84MHz            │
│      │                                                         │
│      └── GPIOF / GPIOx 挂载在 AHB1 总线                       │
│          → 时钟由 RCC->AHB1ENR 控制                            │
│          → __HAL_RCC_GPIOx_CLK_ENABLE() 开启                   │
└─────────────────────────────────────────────────────────────────┘

**关键代码对应：**

| 代码位置                                           | 配置内容                        |
| ---------------------------------------------- | --------------------------- |
| `sys_stm32_clock_init(336, 8, 2, 7)`           | PLL 参数：N=336, M=8, P=2, Q=7 |
| `rcc_clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1` | AHB = SYSCLK/1 = 168MHz     |
| `rcc_clk_init.APB1CLKDivider = RCC_HCLK_DIV4`  | APB1 = AHB/4 = 42MHz        |
| `rcc_clk_init.APB2CLKDivider = RCC_HCLK_DIV2`  | APB2 = AHB/2 = 84MHz        |
| `FLASH_LATENCY_5`                              | 168MHz 需要 5 个 Flash 等待周期    |
| `PWR_REGULATOR_VOLTAGE_SCALE1`                 | 内核电压 1.2V（168MHz 必须）        |
| `__HAL_FLASH_PREFETCH_BUFFER_ENABLE()`         | 使能 Flash 指令预取缓冲             |

**总结：**

- `sys_stm32_clock_init` 封装了 STM32F4 最复杂的时钟配置，调用者只需要传入 4 个参数就能得到想要的频率。

- 跑马灯只需要 GPIO，GPIO 时钟由 `AHB1ENR` 控制，与 APB1/APB2 无关。

- `delay_init(168)` 的参数 168 是 CPU 频率（单位 MHz），用于计算 SysTick 的重载值。

### ② 寄存器级

#### A. 时钟配置（来自 `sys_stm32_clock_init`）

| 寄存器            | 操作                  | 你的代码                                      | 说明           |
| -------------- | ------------------- | ----------------------------------------- | ------------ |
| `RCC->APB1ENR` | 置位 PWREN            | `__HAL_RCC_PWR_CLK_ENABLE()`              | 使能电源管理接口时钟   |
| `PWR->CR`      | VOS=SCALE1          | `__HAL_PWR_VOLTAGESCALING_CONFIG(SCALE1)` | 1.2V 内核电压    |
| `RCC->CR`      | HSEON=1             | `rcc_osc_init.HSEState = RCC_HSE_ON`      | 开启外部 8MHz 晶振 |
| `RCC->PLLCFGR` | PLLN/M/P/Q          | `rcc_osc_init.PLL.PLLN/M/P/Q = ...`       | 配置 PLL 分频倍频  |
| `RCC->CFGR`    | SW/HPRE/PPRE1/PPRE2 | `rcc_clk_init.SYSCLKSource...`            | 系统时钟源 + 总线分频 |
| `FLASH->ACR`   | LATENCY=5           | `FLASH_LATENCY_5`                         | Flash 等待周期   |
| `FLASH->ACR`   | PRFTEN=1            | `__HAL_FLASH_PREFETCH_BUFFER_ENABLE()`    | 指令预取使能       |

#### B. GPIO 配置（来自 实验中的`led_i_init` / `led_e_init`）

| 寄存器              | 操作         | 你的代码中的值                         | 说明                |
| ---------------- | ---------- | ------------------------------- | ----------------- |
| `RCC->AHB1ENR`   | 置位 GPIOFEN | `LED0_GPIOF_CLK_ENABLE()`       | 开启 GPIOF 时钟       |
| `GPIOx->MODER`   | 写入 01      | `GPIO_MODE_OUTPUT_PP`           | 通用输出模式            |
| `GPIOx->OTYPER`  | 写入 0       | 隐含在 `GPIO_MODE_OUTPUT_PP` 中     | 推挽输出              |
| `GPIOx->OSPEEDR` | 写入 00      | `GPIO_SPEED_FREQ_LOW`           | 低速（2MHz，跑马灯够用）    |
| `GPIOx->PUPDR`   | 写入 10      | `GPIO_PULLDOWN`                 | ⚠️ 输出模式下上下拉**无效** |
| `GPIOx->BSRR`    | 置位/复位      | `led_0_on()` 中 `GPIO_PIN_RESET` | 低电平点亮（正点原子板子特性）   |

#### 

### ③ 数据流

**从 `main()` 到物理 LED 发光的完整路径：**

text

┌─────────────────────────────────────────────────────────────────────────┐
│ main.c (App 层)                                                       │
│                                                                       │
│  HAL_Init()                                                           │
│      ├── 配置 Flash 预取 & 缓存                                       │
│      ├── 初始化 SysTick（1ms 时基）                                   │
│      └── 设置 NVIC 优先级分组（4 位抢占）                             │
│                                                                       │
│  sys_stm32_clock_init(336,8,2,7)                                     │
│      └── HAL_RCC_OscConfig() → HAL_RCC_ClockConfig()                 │
│          → 操作 RCC/PWR/FLASH 寄存器 → SYSCLK=168MHz                 │
│                                                                       │
│  delay_init(168)                                                      │
│      └── SysTick_Config(168000000/1000) → 每 1ms 中断一次            │
│                                                                       │
│  usart_init(115200) → 串口初始化（调试用）                            │
│                                                                       │
│  led_i_init() → BSP 层：板载 LED (PF9, PF10)                         │
│      ├── __HAL_RCC_GPIOF_CLK_ENABLE() → 开启 GPIOF 时钟              │
│      ├── HAL_GPIO_Init(GPIOF, &gpio_init_struct) → 配置 MODER/OTYPER/OSPEEDR/PUPDR
│      └── LED0(1); LED1(1); → BSRR 置位 → 高电平 → 默认熄灭           │
│                                                                       │
│  led_e_init() → BSP 层：扩展板 RGB (LEDR/LEDY/LEDG)                  │
│      ├── 开启对应端口时钟                                            │
│      ├── HAL_GPIO_Init() ×3 → 配置三个引脚                          │
│      └── LEDR(0); LEDY(0); LEDG(0); → 低电平 → 默认点亮？           │
│          ⚠️ 注意：这里和板载 LED 刚好相反！                           │
│                                                                       │
│  while(1)                                                             │
│      ├── led_0_on() → HAL_GPIO_WritePin(..., RESET) → BSRR 复位     │
│      ├── led_R_on() → HAL_GPIO_WritePin(..., SET) → BSRR 置位       │
│      ├── delay_ms(500) → HAL_Delay() → SysTick 中断计数             │
│      ├── led_0_off() → BSRR 置位                                     │
│      ├── led_R_off() → BSRR 复位                                     │
│      └── ...                                                         │
└─────────────────────────────────────────────────────────────────────────┘

**💡板载 LED 和扩展板 RGB 的电平逻辑不同！**

| LED           | 点亮函数         | 内部电平                 | 熄灭函数          | 内部电平                 |
| ------------- | ------------ | -------------------- | ------------- | -------------------- |
| 板载 LED0 (PF9) | `led_0_on()` | `GPIO_PIN_RESET` (0) | `led_0_off()` | `GPIO_PIN_SET` (1)   |
| 外设 LEDR       | `led_R_on()` | `GPIO_PIN_SET` (1)   | `led_R_off()` | `GPIO_PIN_RESET` (0) |

板载 LED 是低电平点亮（共阳极），外设 RGB 是高电平点亮（共阴极）



### ④ 中断/DMA 机制

**本实验实际使用的中断：**

| 中断源            | 产生频率     | 作用                                    | 在你的代码中如何体现                                 |
| -------------- | -------- | ------------------------------------- | ------------------------------------------ |
| **SysTick 中断** | 每 1ms 一次 | 累加 `uwTick` 全局变量，为 `HAL_Delay()` 提供时基 | `HAL_Init()` 中初始化，`delay_init(168)` 中设置重载值 |

**中断向量表位置**（由启动文件 `startup_stm32f407xx.s` 定义）：

text

SysTick_Handler    → HAL_IncTick() → uwTick++
HAL_Delay() 内部循环检查 uwTick 差值

**本实验未使用但 HAL 框架支持的中断：**

- 未使用 GPIO 外部中断（EXTI），因为跑马灯只有输出。

- 未使用 DMA，GPIO 输出由 CPU 直接写寄存器，不涉及大数据量搬运。





### ⑤ 改参实验

基于你的三段代码，建议动手验证以下内容：

| 序号  | 改动内容                      | 修改位置                                                                          | 预期现象                           |
| --- | ------------------------- | ----------------------------------------------------------------------------- | ------------------------------ |
| 1   | **合并 LED0/LED1 初始化（同端口）** | `led_i_init()` 中 `Pin = LED0_GPIO_PIN \| LED1_GPIO_PIN`，只调用一次 `HAL_GPIO_Init` | 功能不变，代码量减半                     |
| 2   | **合并 RGB 三色初始化（同端口）**     | `led_e_init()` 中 `Pin = LEDR_GPIO_PIN \| LEDY_GPIO_PIN \| LEDG_GPIO_PIN`      | 功能不变，代码量减少 2/3                 |
| 3   | **修改 `Pull` 为 `NOPULL`**  | 将所有 `GPIO_PULLDOWN` / `GPIO_PULLUP` 改为 `GPIO_NOPULL`                          | LED 表现完全相同，证明输出模式下上下拉无效        |
| 4   | **修改 `Speed` 为 `HIGH`**   | `GPIO_SPEED_FREQ_LOW` → `GPIO_SPEED_FREQ_HIGH`                                | 肉眼看不出来，但高频翻转时（如 PWM）波形会变好      |
| 5   | **修改系统时钟频率**              | `sys_stm32_clock_init(168, 8, 2, 7)` → SYSCLK=84MHz                           | 跑马灯速度不变（`delay_ms` 自动适配），但功耗降低 |

### 

### ⑥踩坑记录

| 序号  | 问题                          | 代码位置                                                                 | 原因                                                          | 解法                                   |
| --- | --------------------------- | -------------------------------------------------------------------- | ----------------------------------------------------------- | ------------------------------------ |
| 1   | **同端口重复开启时钟**               | `led_i_init()` 中 `LED0_GPIO_CLK_ENABLE()` 和 `LED1_GPIO_CLK_ENABLE()` | `LED0_GPIO_PORT` 和 `LED1_GPIO_PORT` 都是 `GPIOF`，相当于开了两次同一个时钟 | 合并为一次 `__HAL_RCC_GPIOF_CLK_ENABLE()` |
| 2   | **输出模式下配置了上下拉**             | `led_i_init()` 中 `Pull = GPIO_PULLDOWN`                              | 可能是从输入模式例程复制过来的，忘记改了                                        | 改为 `GPIO_NOPULL`，因为输出模式上下拉**无效**     |
| 3   | **板载 LED 和 RGB LED 电平逻辑相反** | `led_0_on()` 用 `RESET`，`led_R_on()` 用 `SET`                          | 板载 LED 共阳极（低电平亮），RGB LED 共阴极（高电平亮）                          | 已用函数封装完美处理了！`main.c` 不需要知道这个差异       |
| 5   | **`delay_ms` 阻塞导致无法做其他事**   | `delay_ms(500)`                                                      | 延迟是 CPU 空转，500ms 内无法处理其他任务                                  | 后续引入 FreeRTOS，改用 `vTaskDelay()`      |



## ⑦实验结果视频

https://github.com/user-attachments/assets/b1f57922-2b81-467d-bf4a-64658b0ad503















