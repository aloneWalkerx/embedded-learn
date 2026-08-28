#include "./BSP/TIMER/g-timer.h"
#include "./BSP/LED/led-i.h"


//通用定时器句柄
TIM_HandleTypeDef g_timx_cnt_handle = {0};

//脉冲计数定时器相关变量,脉冲计数定时器溢出计数器
uint32_t g_timx_chy_cnt_ofcnt = 0;

/**
 * @brief   初始化通用定时器脉冲计数
 * @param   psc: 预分频系数
 * @retval  无
 */
void gtim_timx_cnt_chy_init(uint16_t psc)
{
    //TIM从机句柄
    TIM_SlaveConfigTypeDef tim_slave_config_struct = {0};
    
    //对应定时器
    g_timx_cnt_handle.Instance = GTIM_TIMX_CNT;
    
    //预分频器系数
    g_timx_cnt_handle.Init.Prescaler = psc;
    
    //计数模式：递增模式
    g_timx_cnt_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    
    //重装载值
    g_timx_cnt_handle.Init.Period = 0xFFFF; 
    
    //根据参数初始化定时器输入捕获
    HAL_TIM_IC_Init(&g_timx_cnt_handle);
    
    //设置模式为从模式，外部触发
    tim_slave_config_struct.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
    
    //触发源：输入触发,触发源是通道1的输入信号（经过滤波和极性选择后的 TI1FP1）
    tim_slave_config_struct.InputTrigger = TIM_TS_TI1FP1;
    
    //触发极性:上升沿触发
    tim_slave_config_struct.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
    
    //触发分频:不使用分频
    tim_slave_config_struct.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
    
    //触发滤波器为0
    tim_slave_config_struct.TriggerFilter = 0;
    
    //根据参数配置定时器从模式
    HAL_TIM_SlaveConfigSynchro(&g_timx_cnt_handle, &tim_slave_config_struct);
    
    //使能定时器更新中断/使能溢出中断（当计数器从 0xFFFF 溢出到 0 时触发）
    __HAL_TIM_ENABLE_IT(&g_timx_cnt_handle, TIM_IT_UPDATE);
    
    //开启定时器输入捕获
    HAL_TIM_IC_Start(&g_timx_cnt_handle, GTIM_TIMX_CNT_CHY);
}


/**
 * @brief   获取通用定时器脉冲计数值
 * @param   无
 * @retval  脉冲计数值
 */
uint32_t gtim_timx_cnt_chy_get_count(void)
{
    uint32_t total;
    
    /* 计算总脉冲计数值 */
    //g_timx_chy_cnt_ofcnt：溢出次数 ，，0xFFFF：65535
    total = g_timx_chy_cnt_ofcnt * 0xFFFF;
    //__HAL_TIM_GET_COUNTER：TIM计数器寄存器的值
    total += __HAL_TIM_GET_COUNTER(&g_timx_cnt_handle);
    
    return total;
}

/**
 * @brief   重启通用定时器脉冲计数
 * @param   无
 * @retval  无
 */
void gtim_timx_cnt_chy_restart(void)
{
    //关闭定时器（防止写入时出错）
    __HAL_TIM_DISABLE(&g_timx_cnt_handle);
    
    //清零溢出次数
    g_timx_chy_cnt_ofcnt = 0;
    
    //计数器归零
    __HAL_TIM_SET_COUNTER(&g_timx_cnt_handle, 0);
    
    //重新开启定时器
    __HAL_TIM_ENABLE(&g_timx_cnt_handle);
}

/**
 * @brief   HAL库定时器输出捕获初始化MSP函数
 * @param   无
 * @retval  无
 */
void HAL_TIM_IC_MspInit(TIM_HandleTypeDef *htim)
{
    //引脚初始化句柄
    GPIO_InitTypeDef gpio_init_struct;
    
    //如果是脉冲计数器
    if (htim->Instance == GTIM_TIMX_CNT)
    {
        //使能复用引脚对应时钟
        GTIM_TIMX_CNT_CHY_GPIO_CLK_ENABLE();
        
        //使能定时器时钟
        GTIM_TIMX_CNT_CLK_ENABLE();
        
        //设置脉冲引脚
        gpio_init_struct.Pin = GTIM_TIMX_CNT_CHY_GPIO_PIN;
        
        //设置模式为复用推挽输出
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        
        //设置为下拉模式
        gpio_init_struct.Pull = GPIO_PULLDOWN;
        
        //速度设置为高速
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        
        //复用号
        gpio_init_struct.Alternate = GTIM_TIMX_CNT_CHY_GPIO_AF;
        
        //根据参数初始化复用引脚
        HAL_GPIO_Init(GTIM_TIMX_CNT_CHY_GPIO_PORT, &gpio_init_struct);
        
        //配置中断优先级
        HAL_NVIC_SetPriority(GTIM_TIMX_CNT_IRQn, 0, 0);
        
        //使能定时器中断
        HAL_NVIC_EnableIRQ(GTIM_TIMX_CNT_IRQn);
    }
}


/**
 * @brief   脉冲计数通用定时器中断回调函数
 * @param   无
 * @retval  无
 */
void GTIM_TIMX_CNT_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_timx_cnt_handle);
}

/**
 * @brief   HAL库定时器超时中断回调函数
 * @param   无
 * @retval  无
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
   
   
    // 脉冲计数定时器
     if (htim->Instance == GTIM_TIMX_CNT)
    {
        //更新脉冲计数定时器溢出次数
        g_timx_chy_cnt_ofcnt++;
    }
}

