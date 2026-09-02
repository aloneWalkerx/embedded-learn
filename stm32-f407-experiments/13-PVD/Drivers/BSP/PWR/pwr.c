#include "./BSP/PWR/pwr.h"      // 包含 PWR（电源控制）模块的头文件（函数声明、宏定义等）
#include "./BSP/LED/led-i.h"    // 包含 LED 驱动头文件（用于电压异常时点亮 LED1 报警）
#include "./BSP/LCD/lcd.h"      // 包含 LCD 驱动头文件（用于在屏幕上显示电压状态信息）

/* ====================================================================
   ① 初始化 PVD（可编程电压检测器）
   ==================================================================== */
/**
 * @brief   初始化PVD
 * @param   pl: 电压等级（阈值），可选值：
 *   @arg   PWR_PVDLEVEL_0: 2.1V
 *   @arg   PWR_PVDLEVEL_1: 2.2V
 *   @arg   PWR_PVDLEVEL_2: 2.3V
 *   @arg   PWR_PVDLEVEL_3: 2.4V
 *   @arg   PWR_PVDLEVEL_4: 2.5V
 *   @arg   PWR_PVDLEVEL_5: 2.6V
 *   @arg   PWR_PVDLEVEL_6: 2.7V
 *   @arg   PWR_PVDLEVEL_7: 2.9V（最常用）
 * @retval  无
 */
void pwr_pvd_init(uint32_t pl)
{
    /* --- 步骤 1：定义 PVD 配置结构体（全部初始化为 0） --- */
    PWR_PVDTypeDef pwr_pvd_struct = {0};
    
    /* --- 步骤 2：使能电源控制接口（PWR）的时钟 --- */
    /* ★ 必须使能 PWR 时钟，否则无法访问 PVD 相关寄存器 */
    __HAL_RCC_PWR_CLK_ENABLE();
    
    /* --- 步骤 3：配置 PVD 中断的 NVIC（嵌套向量中断控制器） --- */
    /* ★ 设置中断优先级：抢占优先级 0（最高），子优先级 0 */
    HAL_NVIC_SetPriority(PVD_IRQn, 0, 0);
    
    /* ★ 使能 PVD 中断（当电压超过/低于阈值时，CPU 会响应中断） */
    HAL_NVIC_EnableIRQ(PVD_IRQn);
    
    /* --- 步骤 4：配置 PVD 参数 --- */
    /* ① 设置电压阈值（由调用者传入，如 PWR_PVDLEVEL_7 = 2.9V） */
    pwr_pvd_struct.PVDLevel = pl;
    
    /* ② 设置触发模式：
     *    PWR_PVD_MODE_IT_RISING_FALLING = 电压上升沿和下降沿都触发中断
     *    （即电压从高到低 和 从低到高 都会触发中断）
     */
    pwr_pvd_struct.Mode = PWR_PVD_MODE_IT_RISING_FALLING;
    
    /* ③ 将配置写入硬件寄存器 */
    HAL_PWR_ConfigPVD(&pwr_pvd_struct);
    
    /* --- 步骤 5：使能 PVD（开始工作） --- */
    /* ★ 在配置完所有参数后，最后使能 PVD，让电压检测器正式运行 */
    HAL_PWR_EnablePVD();
}

/* ====================================================================
   ② HAL 库 PVD 中断回调函数（由 HAL 库自动调用）
   ==================================================================== */
/**
 * @brief   HAL库PVD中断回调函数
 * @param   无
 * @retval  无
 * @note    此函数由 HAL 库在 PVD 中断发生时自动调用
 *          不要手动调用此函数
 */
void HAL_PWR_PVDCallback(void)
{
    /* --- 步骤 1：读取 PVD 输出标志位（PVDO） --- */
    /* ★ __HAL_PWR_GET_FLAG(PWR_FLAG_PVDO) 读取 PVD 比较器输出
     *    PWR_FLAG_PVDO = 1 → 当前 VDD 电压低于设定的阈值
     *    PWR_FLAG_PVDO = 0 → 当前 VDD 电压高于设定的阈值
     */
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_PVDO) != RESET)  /* PVDO 不为 0 → 电压过低 */
    {
        /* --- 电压过低（低于阈值）时的处理 --- */
        
        /* ① 点亮 LED1（报警指示） */
        /* ★ LED1(0) 表示点亮 LED1（你的硬件是低电平点亮） */
        LED1(0);
        
        /* ② 在 LCD 上显示 "PVD Low Voltage!" 红色提示 */
        /*    位置：(30, 130)，区域宽 200 高 16，字体 16，红色 */
        lcd_show_string(30, 130, 200, 16, 16, "PVD Low Voltage!", RED);
        
        /* ★ 在这里还可以添加其他紧急处理代码，例如：
         *    - 保存关键数据到 Flash/EEPROM
         *    - 关闭非必要外设以降低功耗
         *    - 触发蜂鸣器报警
         */
    }
    else  /* PVDO == 0 → 电压恢复正常 */
    {
        /* --- 电压恢复正常（高于阈值）时的处理 --- */
        
        /* ① 熄灭 LED1（取消报警） */
        /* ★ LED1(1) 表示熄灭 LED1 */
        LED1(1);
        
        /* ② 在 LCD 上显示 "PVD Voltage OK! " 蓝色提示 */
        /*    位置：(30, 130)，区域宽 200 高 16，字体 16，蓝色 */
        lcd_show_string(30, 130, 200, 16, 16, "PVD Voltage OK! ", BLUE);
    }
}

/* ====================================================================
   ③ PVD 中断服务函数（由硬件触发，进入中断向量表）
   ==================================================================== */
/**
 * @brief   PVD中断服务函数
 * @param   无
 * @retval  无
 * @note    此函数由硬件中断自动调用，名称需与中断向量表一致
 *          当 VDD 电压跨越阈值时，硬件自动触发此中断
 *          函数名 "PVD_IRQHandler" 必须与 startup_stm32f407xx.s 中的向量表一致
 */
void PVD_IRQHandler(void)
{
    /* ★ 调用 HAL 库的 PVD 中断处理函数
     *    它会做三件事：
     *    1. 清除中断标志位（防止重复触发）
     *    2. 判断中断来源（PVD 只有这一个中断源，直接通过）
     *    3. 调用 HAL_PWR_PVDCallback() 回调函数（执行用户代码）
     * 
     * ★ 不要在这里直接写业务逻辑，而是统一在 HAL_PWR_PVDCallback 中处理
     *    这样符合 HAL 库的分层设计，也方便代码移植
     */
    HAL_PWR_PVD_IRQHandler();
}

