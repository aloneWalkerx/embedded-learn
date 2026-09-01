#include "./BSP/RTC/rtc.h"      // 包含 RTC 模块的头文件（函数声明、结构体定义等）
#include "./BSP/LED/led-i.h"    // 包含 LED 驱动头文件（用于唤醒中断中翻转 LED1）
#include <stdio.h>              // 包含标准 I/O 库（用于 printf 打印调试信息）

/* ====================================================================
   全局变量定义
   ==================================================================== */
RTC_HandleTypeDef g_rtc_handle = {0};   /* RTC 硬件句柄（HAL 库用来管理 RTC 外设） */

/* ====================================================================
   ① 读取后备寄存器
   ==================================================================== */
/**
 * @brief   读取后备寄存器
 * @param   bkrx: 后备寄存器编号（如 RTC_BKP_DR0 ~ RTC_BKP_DR19）
 * @retval  后备寄存器中的 16 位数据
 */
uint16_t rtc_read_bkr(uint32_t bkrx)
{
    /* HAL_RTCEx_BKUPRead 返回 32 位数据，强制转为 16 位返回 */
    /* 因为本实验只用到低 16 位（存 0x5050/0x5051 这样的标记） */
    return (uint16_t)HAL_RTCEx_BKUPRead(&g_rtc_handle, bkrx);
}

/* ====================================================================
   ② 写入后备寄存器
   ==================================================================== */
/**
 * @brief   写入后备寄存器
 * @param   bkrx: 后备寄存器编号
 * @param   data: 要写入的 16 位数据
 * @retval  无
 */
void rtc_write_bkr(uint32_t bkrx, uint16_t data)
{
    /* ★ 关键：取消备份域的写保护（否则写入失败） */
    HAL_PWR_EnableBkUpAccess();
    
    /* 写入数据到指定的后备寄存器 */
    HAL_RTCEx_BKUPWrite(&g_rtc_handle, bkrx, data);
}

/* ====================================================================
   ③ RTC 初始化（核心函数）
   ==================================================================== */
/**
 * @brief   初始化RTC
 * @param   无
 * @retval  初始化结果
 * @arg     0: 初始化成功
 * @arg     1: 初始化失败
 */
uint8_t rtc_init(void)
{
    uint16_t flag;  /* 用于存储从备份寄存器读出的标记值 */
    
    /* --- 步骤 1：使能 RTC 所需的时钟 --- */
    /* 使能电源管理接口时钟（PWR） */
    __HAL_RCC_PWR_CLK_ENABLE(); 

    // 使能 RTC 外设时钟
    __HAL_RCC_RTC_ENABLE();
    
    //取消备份域写保护（允许操作备份寄存器和 RTC）
    HAL_PWR_EnableBkUpAccess();
    
    /* --- 步骤 2：配置 RTC 核心参数 --- */
    //指定使用 RTC 外设
    g_rtc_handle.Instance = RTC;
    
    //24 小时制
    g_rtc_handle.Init.HourFormat = RTC_HOURFORMAT_24;
    
    //异步预分频：128 分频（7F = 127 + 1）
    g_rtc_handle.Init.AsynchPrediv = 0x7F;
    
    //同步预分频：256 分频（FF = 255 + 1）
    g_rtc_handle.Init.SynchPrediv = 0xFF;
    /* ★ 时钟计算：32.768kHz / 128 / 256 = 1Hz，产生 1 秒的基准时钟 */
    
    //禁用 RTC 输出引脚（如闹钟输出）
    g_rtc_handle.Init.OutPut = RTC_OUTPUT_DISABLE;
    
    //输出极性（默认高电平）
    g_rtc_handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    
    //输出类型：开漏
    g_rtc_handle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    
    /* --- 步骤 3：从备份寄存器 0 读取配置标志（判断是否第一次上电） --- */
    //读取备份寄存器 0 的值
    flag = rtc_read_bkr(0);
    
    /* --- 步骤 4：初始化 RTC 硬件（HAL 库会调用 HAL_RTC_MspInit） --- */
    //如果初始化失败
    if (HAL_RTC_Init(&g_rtc_handle) != HAL_OK)
    {
        return 1;   /* 返回 1 表示失败 */
    }
    
    /* --- 步骤 5：判断是否第一次配置 RTC --- */
    /* 如果 flag 既不是 0x5050（LSE 标记），也不是 0x5051（LSI 标记） */
    if ((flag != 0x5051) && (flag != 0x5050))
    {
        /* ★ 说明是第一次上电（或电池掉电导致数据丢失） */
        /*    设置默认时间：08:00:00（24 小时制，0 = am，1 = pm） */
        rtc_set_time(8, 0, 0, 0);
        
        /*    设置默认日期：2023-04-23 星期日（week=7，0~7） */
        rtc_set_date(23, 4, 23, 7);
        
        /* ★ 注意：实际写入标记是在 HAL_RTC_MspInit 中完成的 */
    }
    /* 如果 flag 是 0x5050 或 0x5051，说明 RTC 已配置过，跳过时间设置 */
    
    return 0;   /* 初始化成功 */
}

/* ====================================================================
   ④ RTC MSP 初始化函数（HAL 库自动调用）
   ==================================================================== */
/**
 * @brief   HAL库RTC初始化MSP函数（由 HAL_RTC_Init 自动调用）
 * @param   hrtc: RTC 句柄指针
 * @retval  无
 */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    //振荡器配置结构体
    RCC_OscInitTypeDef rcc_osc_init_struct = {0};
    
    //外设时钟配置结构体
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init_struct = {0};
    
    if (hrtc->Instance == RTC)   /* 确保是 RTC 外设的 MSP 初始化 */
    {
        /* --- 尝试 1：优先使用 LSE（外部 32.768kHz 晶振） --- */
        rcc_osc_init_struct.OscillatorType = RCC_OSCILLATORTYPE_LSE;  /* 选择 LSE 振荡器 */
        rcc_osc_init_struct.LSEState = RCC_LSE_ON;    /* 开启 LSE */
        rcc_osc_init_struct.PLL.PLLState = RCC_PLL_NONE;  /* PLL 不参与配置 */
        
        if (HAL_RCC_OscConfig(&rcc_osc_init_struct) == HAL_OK)  /* 如果 LSE 配置成功 */
        {
            /* ★ LSE 可用，配置 RTC 时钟源为 LSE */
            rcc_periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_RTC; /* 选择 RTC 外设时钟 */
            rcc_periph_clk_init_struct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE; /* 时钟源 = LSE */
            HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init_struct);  /* 应用配置 */
            
            /* ★ 写入标记 0x5050 到备份寄存器 0（表示使用 LSE） */
            rtc_write_bkr(0, 0x5050);
            
            __HAL_RCC_RTC_ENABLE();   /* 使能 RTC 时钟 */
        }
        else
        {
            /* --- 尝试 2：LSE 不可用，改用 LSI（内部 ~32kHz RC 振荡器） --- */
            rcc_osc_init_struct.OscillatorType = RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_LSI;
            rcc_osc_init_struct.LSEState = RCC_LSE_OFF;   /* 关闭 LSE */
            rcc_osc_init_struct.LSIState = RCC_LSI_ON;    /* 开启 LSI */
            rcc_osc_init_struct.PLL.PLLState = RCC_PLL_NONE;
            HAL_RCC_OscConfig(&rcc_osc_init_struct);      /* 配置振荡器 */
            
            /* ★ LSI 可用，配置 RTC 时钟源为 LSI */
            rcc_periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
            rcc_periph_clk_init_struct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;  /* 时钟源 = LSI */
            HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init_struct);
            
            /* ★ 写入标记 0x5051 到备份寄存器 0（表示使用 LSI） */
            rtc_write_bkr(0, 0x5051);
            
            __HAL_RCC_RTC_ENABLE();   /* 使能 RTC 时钟 */
        }
    }
}

/* ====================================================================
   ⑤ 设置 RTC 时间（可通过 USMART 调用）
   ==================================================================== */
/**
 * @brief   设置RTC时间信息
 * @param   hour: 时（0~23，24小时制）
 * @param   minute: 分（0~59）
 * @param   second: 秒（0~59）
 * @param   ampm: 上下午（仅在 12 小时制时使用，24小时制填 0）
 * @arg     0: 上午
 * @arg     1: 下午
 * @retval  设置结果
 * @arg     0: 设置成功
 * @arg     1: 设置失败
 */
uint8_t rtc_set_time(uint8_t hour, uint8_t minute, uint8_t second, uint8_t ampm)
{
    RTC_TimeTypeDef rtc_time_struct = {0};   /* 时间结构体（全部初始化为 0） */
    
    /* 填充时间结构体 */
    rtc_time_struct.Hours = hour;            /* 时 */
    rtc_time_struct.Minutes = minute;        /* 分 */
    rtc_time_struct.Seconds = second;        /* 秒 */
    rtc_time_struct.TimeFormat = ampm;       /* 上下午（24小时制下被忽略） */
    rtc_time_struct.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;   /* 不启用夏令时 */
    rtc_time_struct.StoreOperation = RTC_STOREOPERATION_RESET;  /* 复位存储操作 */
    
    /* 调用 HAL 库函数设置时间（RTC_FORMAT_BIN 表示使用二进制格式） */
    if (HAL_RTC_SetTime(&g_rtc_handle, &rtc_time_struct, RTC_FORMAT_BIN) != HAL_OK)
    {
        return 1;   /* 设置失败 */
    }
    
    return 0;   /* 设置成功 */
}

/* ====================================================================
   ⑥ 设置 RTC 日期（可通过 USMART 调用）
   ==================================================================== */
/**
 * @brief   设置RTC日期信息
 * @param   year: 年（0~99，代表 2000~2099）
 * @param   month: 月（1~12）
 * @param   date: 日（1~31）
 * @param   week: 星期（1=周一 ~ 7=周日）
 * @retval  设置结果
 * @arg     0: 设置成功
 * @arg     1: 设置失败
 */
uint8_t rtc_set_date(uint8_t year, uint8_t month, uint8_t date, uint8_t week)
{
    RTC_DateTypeDef rtc_date_struct = {0};   /* 日期结构体（全部初始化为 0） */
    
    /* 填充日期结构体 */
    rtc_date_struct.WeekDay = week;   /* 星期 */
    rtc_date_struct.Month = month;    /* 月 */
    rtc_date_struct.Date = date;      /* 日 */
    rtc_date_struct.Year = year;      /* 年（0~99） */
    
    /* 调用 HAL 库函数设置日期 */
    if (HAL_RTC_SetDate(&g_rtc_handle, &rtc_date_struct, RTC_FORMAT_BIN) != HAL_OK)
    {
        return 1;   /* 设置失败 */
    }
    
    return 0;   /* 设置成功 */
}

/* ====================================================================
   ⑦ 获取 RTC 时间（用于 LCD 显示）
   ==================================================================== */
/**
 * @brief   获取RTC时间信息
 * @param   hour: 指向 时 变量的指针
 * @param   minute: 指向 分 变量的指针
 * @param   second: 指向 秒 变量的指针
 * @param   ampm: 指向 上下午 变量的指针
 * @retval  无
 */
void rtc_get_time(uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *ampm)
{
    RTC_TimeTypeDef rtc_time_struct = {0};   /* 时间结构体 */
    
    /* ★ 从 RTC 硬件读取当前时间到结构体 */
    HAL_RTC_GetTime(&g_rtc_handle, &rtc_time_struct, RTC_FORMAT_BIN);
    
    /* ★ 通过指针返回给调用者（main.c 中会用这些值来显示） */
    *hour = rtc_time_struct.Hours;
    *minute = rtc_time_struct.Minutes;
    *second = rtc_time_struct.Seconds;
    *ampm = rtc_time_struct.TimeFormat;
}

/* ====================================================================
   ⑧ 获取 RTC 日期（用于 LCD 显示）
   ==================================================================== */
/**
 * @brief   获取RTC日期信息
 * @param   year: 指向 年 变量的指针
 * @param   month: 指向 月 变量的指针
 * @param   date: 指向 日 变量的指针
 * @param   week: 指向 星期 变量的指针
 * @retval  无
 */
void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *week)
{
    RTC_DateTypeDef rtc_date_struct = {0};   /* 日期结构体 */
    
    /* ★ 从 RTC 硬件读取当前日期到结构体 */
    /* 注意：HAL_RTC_GetDate 必须在 HAL_RTC_GetTime 之后调用（否则时间不更新） */
    HAL_RTC_GetDate(&g_rtc_handle, &rtc_date_struct, RTC_FORMAT_BIN);
    
    /* ★ 通过指针返回给调用者 */
    *year = rtc_date_struct.Year;
    *month = rtc_date_struct.Month;
    *date = rtc_date_struct.Date;
    *week = rtc_date_struct.WeekDay;
}

/* ====================================================================
   ⑨ 年月日转星期（基姆拉尔森公式）
   ==================================================================== */
/**
 * @brief   年月日转星期
 * @param   year: 年（完整 4 位数，如 2024）
 * @param   month: 月（1~12）
 * @param   date: 日（1~31）
 * @retval  星期（0=周日，1=周一 ... 6=周六）
 * @note    这是基姆拉尔森（Kim Larson）计算公式
 */
uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t date)
{
    uint8_t week = 0;
    
    /* 基姆拉尔森公式：把 1 月、2 月视为上一年的 13 月、14 月 */
    if (month < 3)
    {
        month += 12;   /* 1月→13月，2月→14月 */
        --year;        /* 年份减 1 */
    }
    
    /* 套用公式计算星期（0=周日） */
    week = (date + 1 + 2 * month + 3 * (month + 1) / 5 + year + (year >> 2) - year / 100 + year / 400) % 7;
    
    return week;
}

/* ====================================================================
   ⑩ 设置 RTC 闹钟 A（可通过 USMART 调用）
   ==================================================================== */
/**
 * @brief   设置RTC闹钟时间信息
 * @param   week: 星期（1=周一 ~ 7=周日；0 表示每天）
 * @param   hour: 时
 * @param   minute: 分
 * @param   second: 秒
 * @retval  无
 */
void rtc_set_alarm(uint8_t week, uint8_t hour, uint8_t minute, uint8_t second)
{
    RTC_AlarmTypeDef rtc_alarm_struct = {0};   /* 闹钟结构体 */
    
    /* --- 步骤 1：配置闹钟中断的 NVIC（嵌套向量中断控制器） --- */
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);   /* 抢占优先级 0，子优先级 0（最高优先级） */
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);           /* 使能 RTC 闹钟中断 */
    
    /* --- 步骤 2：配置闹钟参数 --- */
    rtc_alarm_struct.AlarmTime.Hours = hour;          /* 时 */
    rtc_alarm_struct.AlarmTime.Minutes = minute;      /* 分 */
    rtc_alarm_struct.AlarmTime.Seconds = second;      /* 秒 */
    rtc_alarm_struct.AlarmTime.TimeFormat = RTC_HOURFORMAT12_AM;  /* 12 小时制格式（AM） */
    rtc_alarm_struct.AlarmTime.SubSeconds = 0;        /* 亚秒（一般不用） */
    
    rtc_alarm_struct.AlarmMask = RTC_ALARMMASK_NONE;  /* ★ 不屏蔽任何字段（精确匹配） */
    rtc_alarm_struct.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_NONE;  /* 不屏蔽亚秒 */
    
    /* ★ 匹配模式：按星期匹配（每周某一天触发） */
    rtc_alarm_struct.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY;
    rtc_alarm_struct.AlarmDateWeekDay = week;          /* 星期几触发（1~7） */
    
    rtc_alarm_struct.Alarm = RTC_ALARM_A;              /* 使用闹钟 A（还有闹钟 B 可用） */
    
    /* --- 步骤 3：使能闹钟中断并写入硬件 --- */
    HAL_RTC_SetAlarm_IT(&g_rtc_handle, &rtc_alarm_struct, RTC_FORMAT_BIN);
}

/* ====================================================================
   ? HAL 库闹钟 A 中断回调（用户代码在此执行）
   ==================================================================== */
/**
 * @brief   HAL库RTC闹钟A中断回调函数（由 HAL 库自动调用）
 * @param   hrtc: RTC句柄
 * @retval  无
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    /* ★ 闹钟触发时执行：打印提示信息到串口 */
    printf("Alarm A!\r\n");
    
    /* 可以在这里添加其他动作，如：闪烁 LED、播放声音等 */
}

/* ====================================================================
   ? RTC 闹钟中断服务函数（由硬件触发，进入中断向量表）
   ==================================================================== */
/**
 * @brief   RTC闹钟中断服务函数
 * @param   无
 * @retval  无
 * @note    此函数由硬件中断自动调用，名称需与中断向量表一致
 */
void RTC_Alarm_IRQHandler(void)
{
    /* 调用 HAL 库的闹钟中断处理函数（它会判断中断源并调用对应的回调） */
    HAL_RTC_AlarmIRQHandler(&g_rtc_handle);
}

/* ====================================================================
   ? 设置 RTC 周期性唤醒中断
   ==================================================================== */
/**
 * @brief   设置RTC周期性唤醒中断
 * @param   clock: 唤醒时钟源
 *   @arg   RTC_WAKEUPCLOCK_CK_SPRE_16BITS: 1Hz 时钟（最常用）
 *   @arg   RTC_WAKEUPCLOCK_CK_SPRE_17BITS: 1Hz 时钟（17位计数器）
 *   @arg   RTC_WAKEUPCLOCK_RTCCLK_DIV16: RTC 时钟 / 16
 *   @arg   RTC_WAKEUPCLOCK_RTCCLK_DIV8: RTC 时钟 / 8
 *   @arg   RTC_WAKEUPCLOCK_RTCCLK_DIV4: RTC 时钟 / 4
 *   @arg   RTC_WAKEUPCLOCK_RTCCLK_DIV2: RTC 时钟 / 2
 *   @arg   RTC_WAKEUPCLOCK_RTCCLK_DIV1: RTC 时钟 / 1
 * @param   count: 唤醒计数器值（0~65535）
 *   @note   唤醒周期 = count / 时钟频率
 *   例如：clock = RTC_WAKEUPCLOCK_CK_SPRE_16BITS（1Hz），count = 1 → 1 秒
 * @retval  无
 */
void rtc_set_wakeup(uint8_t clock, uint8_t count)
{
    /* --- 步骤 1：配置唤醒中断的 NVIC --- */
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);   /* 抢占优先级 0，子优先级 0 */
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);           /* 使能 RTC 唤醒中断 */
    
    /* --- 步骤 2：设置唤醒定时器并开始计数 --- */
    /* 唤醒周期 = count / clock_freq */
    /* 使用 RTC_WAKEUPCLOCK_CK_SPRE_16BITS（1Hz）时，count=0 表示立即触发，count=1 表示 1 秒 */
    HAL_RTCEx_SetWakeUpTimer_IT(&g_rtc_handle, count, clock);
}

/* ====================================================================
   ? HAL 库唤醒中断回调（用户代码在此执行）
   ==================================================================== */
/**
 * @brief   HAL库RTC唤醒中断回调函数（由 HAL 库自动调用）
 * @param   hrtc: RTC句柄
 * @retval  无
 */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    /* ★ 唤醒中断触发时执行：翻转 LED1 状态 */
    LED1_TOGGLE();
    
    /* 可以在这里添加其他周期性任务，如：扫描按键、更新传感器数据等 */
}

/* ====================================================================
   ? RTC 唤醒中断服务函数（由硬件触发，进入中断向量表）
   ==================================================================== */
/**
 * @brief   RTC唤醒中断服务函数
 * @param   无
 * @retval  无
 * @note    此函数由硬件中断自动调用，名称需与中断向量表一致
 */
void RTC_WKUP_IRQHandler(void)
{
    /* 调用 HAL 库的唤醒中断处理函数（它会判断中断源并调用对应的回调） */
    HAL_RTCEx_WakeUpTimerIRQHandler(&g_rtc_handle);
}