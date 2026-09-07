#include "./BSP/IIC/iic.h"
#include "./SYSTEM/delay/delay.h"

/* ====================================================================
   ① IIC 延时函数（控制通信速率）
   ==================================================================== */
/**
 * @brief   IIC延时函数
 * @note    用于控制IIC通信速率
 *          延时 2μs → 约 500kHz 的 SCL 频率
 *          如果延时太短，从机可能反应不过来
 *          如果延时太长，通信速度会变慢
 * @param   无
 * @retval  无
 */
static void iic_delay(void)
{
    delay_us(2);   // ★ 延时 2 微秒（控制 SCL 频率约 250kHz）
                   // 标准 IIC 最高 400kHz，2μs 是比较安全的速率
}

/* ====================================================================
   ② IIC 初始化
   ==================================================================== */
/**
 * @brief   初始化IIC
 * @param   无
 * @retval  无
 * @note    配置 SCL（PB8）和 SDA（PB9）为开漏输出模式
 *          ★ IIC 协议要求必须使用开漏输出 + 上拉电阻！
 *          开漏输出的特点：
 *          - 输出 0：引脚被拉低
 *          - 输出 1：引脚释放（高阻态），由上拉电阻拉到高电平
 *          这样多个设备可以同时使用总线，不会短路
 */
void iic_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};   // GPIO 配置结构体（全部初始化为 0）
    
    /* --- 步骤 1：使能 GPIOB 时钟 --- */
    // ★ 必须使能 GPIO 时钟，否则无法控制引脚
    // SCL（PB8）和 SDA（PB9）都在 GPIOB 上
    IIC_SCL_GPIO_CLK_ENABLE();   // 使能 GPIOB 时钟
    IIC_SDA_GPIO_CLK_ENABLE();   // 使能 GPIOB 时钟（与上面重复，但保留更清晰）
    
    /* --- 步骤 2：配置 SCL 引脚 --- */
    gpio_init_struct.Pin = IIC_SCL_GPIO_PIN;   // PB8
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_OD;   // ★ 开漏输出（关键！）
    gpio_init_struct.Pull = GPIO_NOPULL;            // 无上下拉（靠外部上拉电阻）
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;  // 高速
    HAL_GPIO_Init(IIC_SCL_GPIO_PORT, &gpio_init_struct);
    
    /* --- 步骤 3：配置 SDA 引脚 --- */
    gpio_init_struct.Pin = IIC_SDA_GPIO_PIN;   // PB9
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_OD;   // ★ 开漏输出（关键！）
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(IIC_SDA_GPIO_PORT, &gpio_init_struct);
    
    /* --- 步骤 4：产生停止信号，释放总线 --- */
    // 初始化完成后，确保总线处于空闲状态（SCL=H，SDA=H）
    iic_stop();
}

/* ====================================================================
   ③ 起始信号（START）
   ==================================================================== */
/**
 * @brief   产生IIC起始信号
 * @param   无
 * @retval  无
 * @note    时序要求：SCL=1 时，SDA 从 1→0 跳变
 *          起始信号后，总线被主机占用，从机开始监听
 *          
 *          时序图：
 *          SCL: ──────┐     ┌──────────
 *                      │     │
 *          SDA: ───┐   │     │
 *                  │   │     │
 *                  └───┘     └──────────
 *                   ↑
 *              起始信号
 */
void iic_start(void)
{
    IIC_SDA(1);     // ① 先拉高 SDA（保证空闲状态）
    IIC_SCL(1);     // ② 拉高 SCL（准备发送起始信号）
    iic_delay();    // ③ 等待电平稳定（建立时间）
    IIC_SDA(0);     // ★ ④ SCL=1 时，SDA 从高→低跳变 → 起始信号！
    iic_delay();    // ⑤ 保持起始信号（保持时间）
    IIC_SCL(0);     // ⑥ 拉低 SCL，准备传输数据
    iic_delay();    // ⑦ 等待稳定
}

/* ====================================================================
   ④ 停止信号（STOP）
   ==================================================================== */
/**
 * @brief   产生IIC停止信号
 * @param   无
 * @retval  无
 * @note    时序要求：SCL=1 时，SDA 从 0→1 跳变
 *          停止信号后，总线释放，从机停止监听
 *          
 *          时序图：
 *          SCL: ──────┐     ┌──────────
 *                      │     │
 *          SDA: ───┐   │     │
 *                  │   │     │
 *                  └───┘     └──────────
 *                       ↑
 *                  停止信号
 */
void iic_stop(void)
{
    IIC_SDA(0);     // ① 先拉低 SDA
    iic_delay();    // ② 等待稳定
    IIC_SCL(1);     // ③ 拉高 SCL
    iic_delay();    // ④ 等待稳定
    IIC_SDA(1);     // ★ ⑤ SCL=1 时，SDA 从低→高跳变 → 停止信号！
    iic_delay();    // ⑥ 保持停止信号
}

/* ====================================================================
   ⑤ 等待应答（ACK）
   ==================================================================== */
/**
 * @brief   等待IIC应答信号
 * @param   无
 * @retval  等待结果
 * @arg     0: 等待IIC应答信号成功
 * @arg     1: 等待IIC应答信号失败
 * @note    主机发送完一个字节后，释放 SDA
 *          从机在第 9 个时钟周期将 SDA 拉低，表示“收到”
 *          ★ 如果从机没拉低（SDA=1），说明从机没有响应
 */
uint8_t iic_wait_ack(void)
{
    uint8_t waittime = 0;   // 等待计数器（防止死循环）
    uint8_t rack = 0;       // 应答结果：0=成功，1=失败
    
    /* --- 步骤 1：主机释放 SDA，让从机控制 --- */
    IIC_SDA(1);     // ★ 释放 SDA（输出高电平，实际由上拉电阻拉高）
    iic_delay();    // 等待稳定
    IIC_SCL(1);     // 拉高 SCL（第 9 个时钟周期，从机在这个边沿输出 ACK）
    iic_delay();    // 等待稳定
    
    /* --- 步骤 2：检测 SDA 是否为低电平 --- */
    // ★ 从机应答：SDA 被从机拉低（0）
    // ★ 从机非应答：SDA 保持高电平（1）
    while (IIC_SDA_READ != 0)   // 如果 SDA 一直为高（未应答）
    {
        waittime++;              // 增加等待计数
        if (waittime > 250)      // ★ 超时判断（防止死循环）
        {
            iic_stop();          // 超时 → 发送停止信号，结束通信
            rack = 1;            // 标记失败
            break;               // 跳出循环
        }
    }
    
    /* --- 步骤 3：拉低 SCL，结束应答位 --- */
    IIC_SCL(0);     // 拉低 SCL
    iic_delay();    // 等待稳定
    
    return rack;    // 返回结果：0=成功，1=失败
}

/* ====================================================================
   ⑥ 产生 ACK 信号（主机应答）
   ==================================================================== */
/**
 * @brief   产生IIC ACK信号
 * @param   无
 * @retval  无
 * @note    主机在接收完数据后，拉低 SDA 表示“收到”
 *          告诉从机：继续发送下一个字节
 *          
 *          时序：在 SCL=1 时，SDA=0 即为 ACK
 */
void iic_ack(void)
{
    IIC_SDA(0);     // ① 拉低 SDA（应答信号）
    iic_delay();    // ② 等待稳定
    IIC_SCL(1);     // ③ 拉高 SCL（从机在第 9 个时钟检测 ACK）
    iic_delay();    // ④ 等待稳定
    IIC_SCL(0);     // ⑤ 拉低 SCL，结束应答位
    iic_delay();    // ⑥ 等待稳定
    IIC_SDA(1);     // ⑦ 释放 SDA（恢复空闲状态）
    iic_delay();    // ⑧ 等待稳定
}

/* ====================================================================
   ⑦ 产生 NACK 信号（主机非应答）
   ==================================================================== */
/**
 * @brief   产生IIC NACK信号
 * @param   无
 * @retval  无
 * @note    主机在接收完数据后，保持 SDA 高电平
 *          告诉从机：不要再发了，停止传输
 *          
 *          时序：在 SCL=1 时，SDA=1 即为 NACK
 */
void iic_nack(void)
{
    IIC_SDA(1);     // ① 拉高 SDA（非应答信号）
    iic_delay();    // ② 等待稳定
    IIC_SCL(1);     // ③ 拉高 SCL（从机在第 9 个时钟检测 NACK）
    iic_delay();    // ④ 等待稳定
    IIC_SCL(0);     // ⑤ 拉低 SCL，结束应答位
    iic_delay();    // ⑥ 等待稳定
}

/* ====================================================================
   ⑧ 发送一个字节（8 位）
   ==================================================================== */
/**
 * @brief   IIC发送一个字节
 * @param   data: 待发送的一字节数据
 * @retval  无
 * @note    从最高位（bit7）开始发送，逐位送出
 *          发送完成后，SDA 被释放（等待从机应答）
 */
void iic_send_byte(uint8_t data)
{
    uint8_t t;   // 循环计数器（8 位）
    
    /* --- 步骤 1：逐位发送 8 个 bit（从最高位开始） --- */
    for (t = 0; t < 8; t++)
    {
        // ★ 判断当前最高位是 1 还是 0
        // (data & 0x80) 取出最高位，>>7 移到最低位（0 或 1）
        IIC_SDA((data & 0x80) >> 7);   // 设置 SDA 电平
        iic_delay();                    // 等待数据稳定（建立时间）
        IIC_SCL(1);                    // ★ 拉高 SCL → 从机锁存 SDA 上的数据
        iic_delay();                    // 等待从机采样完成（保持时间）
        IIC_SCL(0);                    // 拉低 SCL，准备发送下一位
        data <<= 1;                    // 左移一位，让下一位成为新的最高位
    }
    
    /* --- 步骤 2：释放 SDA，等待从机应答 --- */
    // ★ 发送完 8 位后，释放 SDA（由上拉电阻拉高）
    // 由 iic_wait_ack() 负责检测从机应答
    IIC_SDA(1);
}

/* ====================================================================
   ⑨ 读取一个字节（8 位）
   ==================================================================== */
/**
 * @brief   IIC读取一个字节
 * @param   ack: 响应类型
 * @arg     0: 发送 NACK（不再接收）
 * @arg     1: 发送 ACK（继续接收）
 * @retval  读取到的一字节数据
 * @note    从最高位（bit7）开始读取，逐位读入
 *          ★ 读取前必须释放 SDA（从机控制 SDA）
 *          ★ 读取后根据 ack 参数决定是否应答
 */
uint8_t iic_read_byte(uint8_t ack)
{
    uint8_t i;           // 循环计数器（8 位）
    uint8_t receive = 0; // 接收到的数据（8 位）
    
    /* --- 步骤 1：逐位读取 8 个 bit（从最高位开始） --- */
    for (i = 0; i < 8; i++)
    {
        receive <<= 1;   // ★ 先左移一位，为当前位腾出空间
                         // 例如：已有 1010，左移 → 10100，再填入当前位
        
        IIC_SCL(1);      // 拉高 SCL（从机在 SCL=1 时驱动 SDA）
        iic_delay();     // 等待数据稳定
        
        // ★ 读取 SDA 电平
        // 如果 SDA=1，则 receive 的最低位设为 1
        // 如果 SDA=0，则 receive 的最低位保持 0（因为前面左移了）
        if (IIC_SDA_READ)
        {
            receive++;   // 相当于 receive = receive | 1（最低位置 1）
        }
        
        IIC_SCL(0);      // 拉低 SCL，准备读取下一位
        iic_delay();     // 等待稳定
    }
    
    /* --- 步骤 2：发送应答或非应答信号 --- */
    // ★ 注意：这里的逻辑是 ack=0 发送 NACK，ack 非 0 发送 ACK
    // 这个命名有点反直觉，但函数设计中就是这样
    if (ack == 0)
    {
        iic_nack();      // 发送 NACK → 告诉从机：不要再发了
    }
    else
    {
        iic_ack();       // 发送 ACK → 告诉从机：继续发送下一个字节
    }
    
    return receive;      // 返回读取到的数据
}
