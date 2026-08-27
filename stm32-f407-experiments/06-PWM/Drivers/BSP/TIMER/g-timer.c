#include "./BSP/TIMER/g-timer.h"
#include "./BSP/LED/led-e.h"
#include "./BSP/LED/led-i.h"



/////////////////////////////////////////-start-通用定时器中断-start-///////////////////////////////////////////////////
//通用定时器句柄
TIM_HandleTypeDef g_timx_int_handle = {0};
/**
 * @brief   初始化通用定时器中断
 * @param   arr: 自动重装载值
 * @param   psc: 预分频系数
 * @retval  无
 */
void gtim_timx_int_init(uint16_t arr, uint16_t psc)
{
    //对应定时器
    g_timx_int_handle.Instance = GTIM_TIMX_INT;
    
    //预分频系数
    g_timx_int_handle.Init.Prescaler = psc;
    
    //计数模式-递增计数器
    g_timx_int_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    
    //重装载值
    g_timx_int_handle.Init.Period = arr;
    
    //根据参数初始化定时器
    HAL_TIM_Base_Init(&g_timx_int_handle);
    
    //开启定时器计数和中断
    HAL_TIM_Base_Start_IT(&g_timx_int_handle);
}

/**
 * @brief   HAL库基本定时器初始化MSP函数
 * @param   无
 * @retval  无
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == GTIM_TIMX_INT)
    {
        /* 使能通用定时器时钟 */
        GTIM_TIMX_INT_CLK_ENABLE();
        
        /* 配置中断优先级并使能中断 */
        HAL_NVIC_SetPriority(GTIM_TIMX_INT_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(GTIM_TIMX_INT_IRQn);
    }
}

/**
 * @brief   计数通用定时器中断服务函数（PWM不需要此方法）
 * @param   无
 * @retval  无
 */
void GTIM_TIMX_INT_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_timx_int_handle);
}

/**
 * @brief   HAL库定时器超时中断回调函数（PWM不需要此方法）
 * @param   无
 * @retval  无
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* 计数定时器 */
    if (htim->Instance == GTIM_TIMX_INT)
    {
        LED1_TOGGLE();
    }
   
}
/////////////////////////////////////////-end-通用定时器中断-end-///////////////////////////////////////////////////



/////////////////////////////////////////-start-通用定时器PWM输出-start-///////////////////////////////////////////////////

//PWM通用定时器句柄
TIM_HandleTypeDef g_timx_pwm_handle = {0};

/**
 * @brief   初始化通用定时器PWM
 * @param   arr: 自动重装载值
 * @param   psc: 预分频系数
 * @retval  无
 */
void gtim_timx_pwm_chy_init(uint16_t arr, uint16_t psc)
{
    //创建输出比较句柄
    TIM_OC_InitTypeDef timx_oc_pwm_struct = {0};
    
    //配置对应的定时器
    g_timx_pwm_handle.Instance = GTIM_TIMX_PWM;
    
    //位置预分频系数
    g_timx_pwm_handle.Init.Prescaler = psc;
    
    //计数模式-递增模式
    g_timx_pwm_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    
    //重装载值
    g_timx_pwm_handle.Init.Period = arr;
    
    //根据参数初始化PWM
    HAL_TIM_PWM_Init(&g_timx_pwm_handle);
    
    //输出比较模式-PWM模式1
    timx_oc_pwm_struct.OCMode = TIM_OCMODE_PWM1;
    
    //设定初始占空比为 50%
    timx_oc_pwm_struct.Pulse = (arr + 1) >> 1;
    
    //有效电平-高电平
    timx_oc_pwm_struct.OCPolarity = TIM_OCPOLARITY_HIGH;
    
    //根据句柄参数配置定时器PWM通道
    HAL_TIM_PWM_ConfigChannel(&g_timx_pwm_handle, &timx_oc_pwm_struct, GTIM_TIMX_PWM_CHX);
    
    //开启定时器PWM输出
    HAL_TIM_PWM_Start(&g_timx_pwm_handle, GTIM_TIMX_PWM_CHX);
}

/**
 * @brief   HAL库定时器PWM初始化MSP函数
 * @param   无
 * @retval  无
 */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    //创建引脚初始化句柄
    GPIO_InitTypeDef gpio_init_struct;
    
    // PWM输出定时器 
    if (htim->Instance == GTIM_TIMX_PWM)
    {
        //使能复用引脚时钟 
        GTIM_TIMX_PWM_CHX_GPIO_CLK_ENABLE();
        //使能定时器时钟
        GTIM_TIMX_PWM_CLK_ENABLE();
        
        //初始化PWM输出引脚 
        gpio_init_struct.Pin = GTIM_TIMX_PWM_CHX_GPIO_PIN;
        
        //设置模式为复用推挽输出
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        
        //设置上拉模式
        gpio_init_struct.Pull = GPIO_PULLUP;
        
        //速度设为高速
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        
        //设置复用号
        gpio_init_struct.Alternate = GTIM_TIMX_PWM_CHX_GPIO_AF;
        
        //根据参数初始化PWM引脚
        HAL_GPIO_Init(GTIM_TIMX_PWM_CHX_GPIO_PORT, &gpio_init_struct);
    }
}
/////////////////////////////////////////-start-通用定时器PWM输出-start-///////////////////////////////////////////////////

