#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./SYSTEM/usart/usart.h"
#include "BSP/CAN/can.h"

//环回自测帧数
#define TEST_LOOP_CNT       100
//自发自收用的 ID
#define TEST_TX_ID          0x123

// 发送缓冲区（静态全局），供环回自测反复填充使用，长度固定为8
static uint8_t s_tx_buf[CAN_DLC_MAX];

/**
 * @brief  CAN2 环回自测：发送 N 帧，再收回并校验，统计成功次数
 * @return 成功完成"发-收-校验"的帧数（0 ~ TEST_LOOP_CNT）
 * @note   需在回环模式下调用（CAN2_Init 的 loopback 参数传 1），
 *         发送的帧会被自己收到，用于验证收发链路是否正常
 */
static uint16_t can_test_loopback(void)
{
    // 存放接收到的帧ID
    uint16_t id;
    // 接收数据缓冲区（8字节）
    uint8_t buf[CAN_DLC_MAX];
    // 存放接收到的数据长度（DLC）
    uint8_t len;
    // 循环变量：遍历数据字节
    uint8_t i;
    // 成功计数：发-收-校验全部通过的帧数
    uint16_t ok = 0;
    // 循环变量：测试帧序号
    uint16_t k;

    printf("\r\n---- 环回自测 ----\r\n");

    // 配置滤波器为全部放行（环回时也要过滤波器才能收到）
    CAN2_Filter_AcceptAll();
    // 循环发送 TEST_LOOP_CNT 帧
    for (k = 0; k < TEST_LOOP_CNT; k++) {
        // 遍历8个数据字节
        for (i = 0; i < CAN_DLC_MAX; i++) {
            // 填充测试数据：第k帧第i字节 = k+i（便于校验
            s_tx_buf[i] = (uint8_t) (k + i);
        }
        // 发送一帧标准数据帧（ID=TEST_TX_ID，8字节）
        if (CAN2_SendStd(TEST_TX_ID, s_tx_buf, CAN_DLC_MAX) != CAN_OK) {
            // 发送失败打印序号
            printf("send fail at %u\r\n", k);
            // 本帧失败，跳过接收，进入下一帧
            continue;
        }
        // 延时2ms，等回环数据进入FIFO（回环是瞬时的，但留余量更稳）
        delay_ms(2);
        // 尝试从FIFO0读一帧回来
        if (CAN2_RecvStd(&id, buf, &len) == CAN_OK) {
            // 校验：ID、长度、首字节，3项全对才算通过
            if ((id == TEST_TX_ID) && (len == CAN_DLC_MAX) && (buf[0] == s_tx_buf[0])) {
                // 三项校验全通过，成功计数加1
                ok++;
            }
        }
        // 没收到数据（回环模式下不应发生），打印提示
        else {
            // 没收到数据（回环模式下不应发生），打印提示
            printf("recv empty at %u\r\n", k);
        }
    }
    // 打印成功数/总数
    printf("环回结果: %u/%u\r\n", ok, TEST_LOOP_CNT);
    // 返回成功帧数，供调用者判断测试是否通过
    return ok;
}

int main(void)
{

    /* ① HAL 库初始化
     * 内部做：复位所有外设、初始化 Flash 接口、配置 SysTick 为 1ms 中断
     * 【必须放最前面】—— 后面的 HAL_Delay / delay 都依赖它配好的 SysTick */
    HAL_Init();                                     // 初始化 HAL 库，配置 SysTick 为 1ms 中断

    /* ② 配置系统时钟到 168MHz
     * 参数：sys_stm32_clk_init(PLLN, PLLM, PLLP, PLLQ)，板载 HSE = 8MHz
     *
     *   算式：
     *     VCO 输入 = HSE / PLLM = 8 / 8    = 1 MHz
     *     VCO 输出 = 1MHz × PLLN = 1 × 336 = 336 MHz
     *     SYSCLK   = VCO / PLLP  = 336 / 2  = 168 MHz  ?（F407 最高主频）
     *     PLLQ 输出 = 336 / 7               = 48 MHz   ?（USB/SDIO/RNG 用）
     *
     * 分频结果（这就是"时钟树"）：
     *     AHB  = 168MHz  （GPIO 挂 AHB1）
     *     APB1 = 42MHz   （USART2/3、I2C、CAN 挂这里）
     *     APB2 = 84MHz   （USART1、SPI1 挂这里）
     *
     * ?? 进阶细节：ST 建议 VCO 输入取 2MHz（降低 PLL 抖动），这里是 1MHz
     *    在规格内（1~2MHz）但非最优。要改：sys_stm32_clk_init(168, 4, 2, 7) */
    sys_stm32_clk_init(336, 8, 2, 7);               // 配置系统时钟：PLLN=336, PLLM=8, PLLP=2, PLLQ=7，得到 168MHz

    /* ③ 初始化延时函数，传入系统时钟频率（单位 MHz）
     * 内部用 SysTick 或 TIM 实现 delay_us / delay_ms
     * 这里必须传 168 —— 传错了延时就不准 */
    delay_init(168);                                // 初始化延时函数，传入系统时钟频率 168MHz
    //初始化LED
    can_led_init();
    //初始化串口
    can_usartx_init(115200);
    //初始化CAN
    CAN2_Init(CAN_500K, 1);
    //环回自测
    can_test_loopback();

    while (1) {

    }

}
