#include "./BSP/TIM/btim.h"
#include "./BSP/LED/led.h"

TIM_HandleTypeDef g_btim_handle = {0};              // 定时器句柄，清零 → gState = RESET，HAL 才会回调 MspInit

/**
 * @brief  基本定时器初始化
 * @param  arr : 自动重装载值（0~65535），实际计数 = arr + 1
 * @param  psc : 预分频系数（0~65535），实际分频 = psc + 1
 * @note   定时周期 T = (psc + 1) * (arr + 1) / TIMx_CLK
 *         TIM6 挂在 APB1，主频 168MHz 时 TIM6 时钟 = 84MHz
 */
void btim_init(uint16_t arr, uint16_t psc) {
    g_btim_handle.Instance = BTIM_TIMX;             // 选择定时器实例：TIM6
    g_btim_handle.Init.Period = arr;                // 设置自动重装载值 ARR
    g_btim_handle.Init.Prescaler = psc;             // 设置预分频系数 PSC

    HAL_TIM_Base_Init(&g_btim_handle);              // 初始化定时器
    // 内部：gState==RESET 时回调 HAL_TIM_Base_MspInit
    //       配置 CR1、PSC、ARR 等寄存器
    //       设置 gState = READY

    HAL_TIM_Base_Start_IT(&g_btim_handle);          // 启动定时器并开启更新中断
    // 内部：__HAL_TIM_ENABLE_IT(UPDATE) → UIE = 1
    //       __HAL_TIM_ENABLE()           → CEN = 1
}

/**
 * @brief  MSP 底层初始化，被 HAL_TIM_Base_Init 自动回调
 * @param  btim : 定时器句柄指针
 * @note   职责：开时钟、配 NVIC。基本定时器没有 GPIO 需要配置
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *btim) {
    if (btim->Instance == BTIM_TIMX) {              // 确认是 TIM6 触发的回调
        BTIM_TIMX_CLK_ENABLE();                     // 使能 TIM6 时钟（不开时钟，寄存器写不进去）
        HAL_NVIC_SetPriority(BTIM_TIMX_IRQn, 0, 0); // 抢占优先级 0，子优先级 0
        HAL_NVIC_EnableIRQ(BTIM_TIMX_IRQn);         // 使能 TIM6 中断（不开 NVIC，中断进不来）
    }
}

/**
 * @brief  中断服务函数
 * @note   函数名展开为 TIM6_DAC_IRQHandler，必须和启动文件一致
 *         硬件中断触发后，CPU 跳到这里执行
 */
void BTIM_TIMX_IRQHandler(void) {
    HAL_TIM_IRQHandler(&g_btim_handle);             // 交给 HAL 统一分发
    // 内部：检查 SR 寄存器，判断是哪种事件
    //       更新事件 → 清 UIF → 回调 PeriodElapsed
    //       捕获事件 → 清 CCxIF → 回调 CaptureCallback
}

/**
 * @brief  定时器更新中断回调（每 0.5s 进一次）
 * @param  btim : 定时器句柄指针
 * @note   这里是 HAL 分层回调的最后一层，业务逻辑写在这里
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *btim) {
    if (btim->Instance == BTIM_TIMX) {              // 确认是 TIM6 的更新事件
        LED1_TOGGLE();                              // 翻转 LED1
        // 每 0.5s 翻转一次 → 亮 0.5s + 灭 0.5s
        // → 完整周期 1s → 闪烁频率 1Hz
    }
}
