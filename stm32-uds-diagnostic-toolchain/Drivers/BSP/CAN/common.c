#include "./BSP/CAN/common.h"
#include "./BSP/CAN/can1.h"
#include "./BSP/CAN/can2.h"
#include "./SYSTEM/sys/sys.h"
#include "./BSP/CAN/isotp.h"

//CAN1接收ID
volatile uint16_t g_can1_rx_id;
//CAN1接收长度
volatile uint8_t g_can1_rx_len;
//CAN1接收缓冲
volatile uint8_t g_can1_rx_buf[8];
//CAN1接收计数器
volatile uint32_t g_can1_rx_cnt;

//CAN2接收ID
volatile uint16_t g_can2_rx_id;
//CAN2接收长度
volatile uint8_t g_can2_rx_len;
//CAN2接收缓冲
volatile uint8_t g_can2_rx_buf[8];
//CAN2接收计数器
volatile uint32_t g_can2_rx_cnt;

/**
 * @brief  CAN外设MSP初始化（HAL在HAL_CAN_Init内部自动回调）
 * @param  hcan: CAN句柄指针
 * @note   负责时钟使能与GPIO复用配置，仅对CAN2生效
 */
void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan)
{

    // 定义GPIO初始化结构体并清零
    GPIO_InitTypeDef gpio_init = {0};

    // 判断是否为CAN1实例
    if (hcan->Instance == CAN1) {
        // 使能GPIOA时钟（PA11/PA12需要）。注意：这里必须是 GPIOA，不能用 CAN2 的 GPIOB 宏
        CAN1_GPIO_PORT_CLK_ENABLE();
        //使能CAN1时钟
        CAN1_CLK_ENABLE();
        // 同时配置接收和发送引脚,PB12=RX, PB13=TX
        gpio_init.Pin = CAN1_GPIO_TX_PIN | CAN1_GPIO_RX_PIN;
        // 复用推挽输出模式
        gpio_init.Mode = GPIO_MODE_AF_PP;
        //上拉（CAN RX通常需要上拉）
        gpio_init.Pull = GPIO_PULLUP;
        // 最高速度档（对应OSPEEDR=11）
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        // 复用功能选择AF9（CAN2）
        gpio_init.Alternate = CAN1_GPIO_AF;
        // 按上述参数初始化GPIOB
        HAL_GPIO_Init(CAN1_GPIO_PORT, &gpio_init);

        // 不是CAN2则直接返回（本驱动只处理CAN2）
        return;
    }

    // 判断是否为CAN2实例
    if (hcan->Instance != CAN2) {
        // 不是CAN2则直接返回（本驱动只处理CAN2）
        return;
    }
    // 使能GPIOB时钟（PB12/PB13需要）
    CAN2_GPIO_PORT_CLK_ENABLE();
    //使能CAN1时钟（CAN2从控制器依赖CAN1主控制器的时钟）,只用 CAN2 也必须开
    CAN1_CLK_ENABLE();
    // 使能CAN2时钟
    CAN2_CLK_ENABLE();
    // 同时配置接收和发送引脚,PB12=RX, PB13=TX
    gpio_init.Pin = CAN2_GPIO_TX_PIN | CAN2_GPIO_RX_PIN;
    // 复用推挽输出模式
    gpio_init.Mode = GPIO_MODE_AF_PP;
    //上拉（CAN RX通常需要上拉）
    gpio_init.Pull = GPIO_PULLUP;
    // 最高速度档（对应OSPEEDR=11）
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    // 复用功能选择AF9（CAN2）
    gpio_init.Alternate = CAN2_GPIO_AF;
    // 按上述参数初始化GPIOB
    HAL_GPIO_Init(CAN2_GPIO_PORT, &gpio_init);

}

/**
 * @brief  CAN FIFO0 消息挂起回调（HAL 在中断处理完后自动调用）
 * @param  hcan: 触发此回调的 CAN 句柄指针，可用 hcan->Instance 区分 CAN1 / CAN2
 * @note   这是 HAL 库在 HAL_CAN_IRQHandler 内部识别到 FIFO0 有帧后自动调用的回调。
 *         分两路处理：
 *         - CAN1：把帧存入 g_can1_rx_id / g_can1_rx_len / g_can1_rx_buf，计数 g_can1_rx_cnt++
 *         - CAN2：存入 g_can2_rx_* 并计数，同时把帧交给 isotp_rx_frame() 做 ISO-TP 拼包
 *         数据长度做越界保护（最多 8 字节），防止数组越界。
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // 接收到的帧 ID
    uint16_t id;
    // buf：临时数据缓冲；len：数据长度；i：循环变量
    uint8_t buf[8], len, i;

    // 判断中断来自 CAN1，CAN1只是用于此前增加节点测试，主要以CAN2为主
    if (hcan->Instance == CAN1) {
        // 从 CAN1 的 FIFO0 取一帧，成功则保存到全局变量
        if (CAN1_RecvStd(&id, buf, &len) == CAN_OK) {
            // 保存 ID
            g_can1_rx_id = id;
            // 保存长度
            g_can1_rx_len = len;
            // 逐字节拷贝数据到全局缓冲，最多 8 字节防止越界
            for (i = 0; i < len && i < 8; i++) {
                g_can1_rx_buf[i] = buf[i];
            }
            // 接收计数器加 1
            g_can1_rx_cnt++;
        }
        // CAN1 处理完直接返回，不走下面的 CAN2 逻辑
        return;
    }

    // 中断既不是 CAN1 也不是 CAN2，直接返回（容错）
    if (hcan->Instance != CAN2) {
        return;
    }

    // 从 CAN2 的 FIFO0 取一帧，成功则保存并交给 ISO-TP 拼包
    if (CAN2_RecvStd(&id, buf, &len) == CAN_OK) {
        // 保存 ID
        g_can2_rx_id = id;
        // 保存长度
        g_can2_rx_len = len;
        // 逐字节拷贝数据到全局缓冲，最多 8 字节防止越界
        for (i = 0; i < len && i < 8; i++) {
            g_can2_rx_buf[i] = buf[i];
        }
        // 接收计数器加 1
        g_can2_rx_cnt++;

        //把这一帧交给 ISO-TP 状态机：判断是 SF/FF/CF/FC，拼包或置 FC 标志
        isotp_rx_frame(id, buf, len);
    }
}

//void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
//{
//
//    if (hcan->Instance == CAN1) {
//        //定义一个结构体，用来装收到的ID、长度、帧类型等
//        CAN_RxHeaderTypeDef rxHeader;
//        // 定义一个 8 字节的数组，用来装数据
//        uint8_t buf[8], i;

//        // 从 CAN 控制器的 FIFO0 中把消息读出来，存放到 rxHeader 结构体和 buf 数组中
//        // 如果读取失败（比如没有消息或者硬件错误），直接退出回调函数
//        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, buf) != HAL_OK) {
//            return;
//        }

//        // 获取发送方的 ID（Identifier，标识符）。
//        // 判断是标准帧还是扩展帧：如果是标准帧(CAN_ID_STD)取 StdId(11位)，否则取 ExtId(29位)。最后强转成 16 位保存。
//        g_can1_rx_id = (uint16_t) (rxHeader.IDE == CAN_ID_STD ? rxHeader.StdId : rxHeader.ExtId);

//        // 获取数据的长度 DLC（Data Length Code，数据长度编码）
//        g_can1_rx_len = (uint8_t)rxHeader.DLC;

//        // 安全检查：CAN 协议规定最多 8 个字节，如果读到的长度大于 8，强制改成 8，防止数组越界（内存踩踏）
//        if (g_can1_rx_len > 8u) {
//            g_can1_rx_len = 8u;
//        }

//        // 把收到的数据从临时缓冲区 buf 复制到全局变量 g_rx_buf 中，方便主程序使用
//        for (i = 0; i < g_can1_rx_len; i++) {
//            g_can1_rx_buf[i] = buf[i];
//        }

//        // 接收计数器加 1，用于统计一共收到了多少条消息
//        g_can1_rx_cnt++;
//    }

//    if (hcan->Instance == CAN2) {

//        //定义一个结构体，用来装收到的ID、长度、帧类型等
//        CAN_RxHeaderTypeDef rxHeader;
//        // 定义一个 8 字节的数组，用来装数据
//        uint8_t buf[8], i;

//        // 从 CAN 控制器的 FIFO0 中把消息读出来，存放到 rxHeader 结构体和 buf 数组中
//        // 如果读取失败（比如没有消息或者硬件错误），直接退出回调函数
//        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, buf) != HAL_OK) {
//            return;
//        }

//        // 获取发送方的 ID（Identifier，标识符）。
//        // 判断是标准帧还是扩展帧：如果是标准帧(CAN_ID_STD)取 StdId(11位)，否则取 ExtId(29位)。最后强转成 16 位保存。
//        g_can2_rx_id = (uint16_t) (rxHeader.IDE == CAN_ID_STD ? rxHeader.StdId : rxHeader.ExtId);

//        // 获取数据的长度 DLC（Data Length Code，数据长度编码）
//        g_can2_rx_len = (uint8_t)rxHeader.DLC;

//        // 安全检查：CAN 协议规定最多 8 个字节，如果读到的长度大于 8，强制改成 8，防止数组越界（内存踩踏）
//        if (g_can2_rx_len > 8u) {
//            g_can2_rx_len = 8u;
//        }

//        // 把收到的数据从临时缓冲区 buf 复制到全局变量 g_rx_buf 中，方便主程序使用
//        for (i = 0; i < g_can2_rx_len; i++) {
//            g_can2_rx_buf[i] = buf[i];
//        }

//        // 接收计数器加 1，用于统计一共收到了多少条消息
//        g_can2_rx_cnt++;
//    }

//}
