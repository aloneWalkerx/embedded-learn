#include "./BSP/TIMER/g-timer.h"
#include "./BSP/LED/led-i.h"

// 通用定时器句柄 
TIM_HandleTypeDef g_timx_cap_handle = {0};
/* 输入捕获定时器相关变量 */
uint8_t g_timx_chy_cap_sta = 0;     /* [7]: 捕获完成 [6]: 捕获到高电平 [5:0]: 捕获计数值溢出计数器 */
uint16_t g_timx_chy_cap_val = 0;    /* 捕获的计数值 */


/**
 * @brief   初始化通用定时器输入捕获
 * @param   arr: 自动重装载值
 * @param   psc: 预分频系数
 * @retval  无
 */
void gtim_timx_cap_chy_init(uint16_t arr, uint16_t psc)
{
    //创建输入捕获初始化句柄
    TIM_IC_InitTypeDef timx_ic_cap_struct = {0};
    
    //对应定时器
    g_timx_cap_handle.Instance = GTIM_TIMX_CAP;
    
    //预分频器系数
    g_timx_cap_handle.Init.Prescaler = psc;
    
    //计数模式-递增计数器
    g_timx_cap_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    
    //重装载值
    g_timx_cap_handle.Init.Period = arr;
    
    //根据参数初始化定时器输入捕获
    HAL_TIM_IC_Init(&g_timx_cap_handle);
    
    //触发沿-上升沿
    timx_ic_cap_struct.ICPolarity = TIM_ICPOLARITY_RISING;
    
    //映射选择-直接映射
    timx_ic_cap_struct.ICSelection = TIM_ICSELECTION_DIRECTTI;
    
    //输入捕获分频器-不分频
    timx_ic_cap_struct.ICPrescaler = TIM_ICPSC_DIV1;
    
    //输入滤波
    timx_ic_cap_struct.ICFilter = 0;
    
    //配置定时器输入捕获通道
    HAL_TIM_IC_ConfigChannel(&g_timx_cap_handle, &timx_ic_cap_struct, GTIM_TIMX_CAP_CHY);
    
    //使能定时器更新中断
    __HAL_TIM_ENABLE_IT(&g_timx_cap_handle, TIM_IT_UPDATE);
    
    //开启定时器输入捕获
    HAL_TIM_IC_Start_IT(&g_timx_cap_handle, GTIM_TIMX_CAP_CHY);
}


/**
 * @brief   HAL库定时器输入捕获初始化MSP函数
 * @param   无
 * @retval  无
 */
void HAL_TIM_IC_MspInit(TIM_HandleTypeDef *htim)
{
    //引脚初始化句柄
    GPIO_InitTypeDef gpio_init_struct;
    
    //对应定时器执行对应的逻辑
    if (htim->Instance == GTIM_TIMX_CAP)
    {
       //使能复用对应的引脚时钟
        GTIM_TIMX_CAP_CHY_GPIO_CLK_ENABLE();
        
        //使能对应的定时器时钟
        GTIM_TIMX_CAP_CLK_ENABLE();
        
        //初始化输入捕获引脚
        gpio_init_struct.Pin = GTIM_TIMX_CAP_CHY_GPIO_PIN;
        
        //复用推挽输出
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        
        //下拉模式
        gpio_init_struct.Pull = GPIO_PULLDOWN;
        
        //速度为高速
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        
        //设置定时器对应的复用号
        gpio_init_struct.Alternate = GTIM_TIMX_CAP_CHY_GPIO_AF;
        
        //根据参数初始化引脚
        HAL_GPIO_Init(GTIM_TIMX_CAP_CHY_GPIO_PORT, &gpio_init_struct);
        
        //配置中断优先级
        HAL_NVIC_SetPriority(GTIM_TIMX_CAP_IRQn, 0, 0);
        
        //使能中断
        HAL_NVIC_EnableIRQ(GTIM_TIMX_CAP_IRQn);
    }
    
   
}



/**
 * @brief   输入捕获通用定时器中断回调函数
 * @param   无
 * @retval  无
 */
void GTIM_TIMX_CAP_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_timx_cap_handle);
}


/**
 * @brief   HAL库定时器超时中断回调函数
 * @param   无
 * @retval  无
 * 执行时机：当 TIM5 的计数器从 65535 溢出到 0 的时候触发,如果按键按得时间很短（<65ms），这个函数根本不会执行。
 * 只有当你按住按键超过 65ms，它才会每 65ms 执行一次。检测按键按下时间是否溢出，超出范围，
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  
    //对应输入捕获定时器
    if (htim->Instance == GTIM_TIMX_CAP)
    {
        //上一次按键状态为捕获未结束
        if ((g_timx_chy_cap_sta & 0x80) == 0)
        {
            //上一次按键状态是已捕获到上升沿/按下，那么本次就是抬起
            if ((g_timx_chy_cap_sta & 0x40) != 0)
            {
                //捕获计数值溢出计数器已满，进行异常处理
                if ((g_timx_chy_cap_sta & 0x3F) == 0x3F)
                {
                    //重置指定通道的捕获极性配置（清空当前设置的边沿触发方式）
                    TIM_RESET_CAPTUREPOLARITY(&g_timx_cap_handle, GTIM_TIMX_CAP_CHY);
                    
                    //将捕获极性设置为 上升沿触发
                    TIM_SET_CAPTUREPOLARITY(&g_timx_cap_handle, GTIM_TIMX_CAP_CHY, TIM_ICPOLARITY_RISING);
                    
                    //将状态变量的 bit7（0x80）置 1，强制标记为“捕获完成”。
                    g_timx_chy_cap_sta |= 0x80;
                    
                    //将捕获到的计数值强行设置为 最大值65535（0xFFFF）
                    g_timx_chy_cap_val = 0xFFFF;
                }
                
                //捕获计数值溢出计数器未满
                else
                {
                    /* 更新捕获计数值溢出计数器 */
                    g_timx_chy_cap_sta++;
                }
            }
        }
    }
    
}

/**
 * @brief   HAL库定时器输入捕获中断回调函数
 * @param   无
 * @retval  无
 *          执行时机： 按下，抬起都会走这个函数，因为按下后会从上升沿设置为下降沿，抬起后会从下降沿设置为上升沿
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == GTIM_TIMX_CAP)
    {
        //上一次按键状态是捕获未结束
        if ((g_timx_chy_cap_sta & 0x80) == 0)
        {
            //上一次按键状态是已捕获到上升沿/按下，那么本次就是抬起
            if ((g_timx_chy_cap_sta & 0x40) != 0)
            {
                // 标记为捕获完成并重新开启输入捕获
                g_timx_chy_cap_sta |= 0x80; 
                
                //读取定时器的捕获/比较寄存器（CCR）中的值，存入变量 g_timx_chy_cap_val，即高电平持续的时间长度
                g_timx_chy_cap_val = HAL_TIM_ReadCapturedValue(&g_timx_cap_handle, GTIM_TIMX_CAP_CHY);
                
                //重置指定通道的捕获极性配置（清空当前设置的边沿触发方式）
                TIM_RESET_CAPTUREPOLARITY(&g_timx_cap_handle, GTIM_TIMX_CAP_CHY);
                
                //将捕获极性设置为上升沿触发，为下一次按下按键做准备
                TIM_SET_CAPTUREPOLARITY(&g_timx_cap_handle, GTIM_TIMX_CAP_CHY, TIM_ICPOLARITY_RISING);
            }
            //上一次按键状态为未捕获到上升沿，即是下降沿，为捕获完成，那么本次则是上升沿，按键按下
            else
            {
                //从捕获到第一个上升沿开始计数
                //清空所有状态标志
                g_timx_chy_cap_sta = 0;
                
                //清空捕获值
                g_timx_chy_cap_val = 0;
                
                //标记“已捕获到上升沿”
                g_timx_chy_cap_sta |= 0x40;
                
                //暂时关闭定时器
                __HAL_TIM_DISABLE(&g_timx_cap_handle);
                
                //计数器归零（防止在清零计数器的瞬间发生冲突）
                __HAL_TIM_SET_COUNTER(&g_timx_cap_handle, 0);
                
                //重置指定通道的捕获极性配置（清空当前设置的边沿触发方式）
                TIM_RESET_CAPTUREPOLARITY(&g_timx_cap_handle, GTIM_TIMX_CAP_CHY);
                
                //将捕获极性设置为 下降沿触发，等待按键松开
                TIM_SET_CAPTUREPOLARITY(&g_timx_cap_handle, GTIM_TIMX_CAP_CHY, TIM_ICPOLARITY_FALLING);
                
                //重新开启定时器（开始计时）
                __HAL_TIM_ENABLE(&g_timx_cap_handle);
            }
        }
    }
}
