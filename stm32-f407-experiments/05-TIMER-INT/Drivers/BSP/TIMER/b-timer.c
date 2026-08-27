#include  "./BSP/LED/led-i.h"
#include  "./BSP/TIMER/b-timer.h"

//创建基本定时器实例
TIM_HandleTypeDef  g_timx_handle = {0};

/**
 * @brief   初始化基本定时器
 * @param   prd: 自动重装载值
 * @param   psc: 预分频系数
 * @retval  无
 * (PSC + 1) × (ARR + 1) = 84,000,000 / 目标频率
 */
void btim_timx_int_init(uint16_t prd, uint16_t psc)
{
    //设置具体定时器
    g_timx_handle.Instance = BTIM_TIMX_INT;
    
    //设置预分频系数
    g_timx_handle.Init.Prescaler = psc;
    
    //设置自动重装载值
    g_timx_handle.Init.Period = prd;
    
    //根据对应参数初始化基本定时器
    HAL_TIM_Base_Init(&g_timx_handle);
    
    //于开启TIM中断模式计数,启动定时器 + 使能中断
    HAL_TIM_Base_Start_IT(&g_timx_handle);
}

/**
 * @brief   HAL库TIM初始化MSP函数
 * @param   无
 * @retval  无
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    //如果定时器为TIM6
   if (htim->Instance == BTIM_TIMX_INT){
       //使能基本定时器时钟
       BTIM_TIMX_INT_CLK_ENABLE();
       //设置中断优先级
       HAL_NVIC_SetPriority(BTIM_TIMX_INT_IRQn, 0, 0);
       //使能中断
       HAL_NVIC_EnableIRQ(BTIM_TIMX_INT_IRQn);
       
   
   }
 
}

/**
 * @brief   基本定时器中断服务函数
 * @param   无
 * @retval  无
 */
void BTIM_TIMX_INT_IRQHandler()
{
    //处理定时器中断请求
    HAL_TIM_IRQHandler(&g_timx_handle);

}

static uint8_t cnt = 0;   // 软件计数器
/**
 * @brief   HAL库基本定时器超时中断回调函数
 * @param   无
 * @retval  无
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{   
    //如果是TIM6定时器则进行LED1闪烁
    if(htim->Instance == BTIM_TIMX_INT){
        cnt++;            // 每 500ms 加 1
        if (cnt >= 2) {   // 加到 2 次 = 1 秒
            cnt = 0;
            // ★ 在这里写你想要的“每秒执行一次”的动作
            LED1_TOGGLE();   // 举例：每秒翻转一次 LED0
        }
    }

}

