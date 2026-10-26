#include "./SYSTEM/sys/sys.h"

/* --------------------------- 引脚定义 ------------------------------- */
// CAN2 所用GPIO端口为GPIOB
#define CAN2_GPIO_PORT                   GPIOB
// CAN2 接收引脚为PB12
#define CAN2_GPIO_RX_PIN                 GPIO_PIN_12
// CAN2 发送引脚为PB13
#define CAN2_GPIO_TX_PIN                 GPIO_PIN_13
// 引脚复用功能选择AF9（对应CAN2）
#define CAN2_GPIO_AF                     GPIO_AF9_CAN2

/* --------------------------- 使能时钟 ------------------------------- */
//使能GPIOB时钟
#define CAN2_GPIO_PORT_CLK_ENABLE()      do { __HAL_RCC_GPIOB_CLK_ENABLE();}while(0)
//使能CAN2
#define CAN2_CLK_ENABLE()                do { __HAL_RCC_CAN2_CLK_ENABLE();}while(0)

// 声明CAN2的HAL句柄（在can.c中定义）
extern CAN_HandleTypeDef hcan2;

/* 函数声明 */
//初始化 CAN2（时钟、引脚、位时序）
void CAN2_Init(uint8_t baud_sel, uint8_t loopback);
//发送一帧标准数据帧
uint8_t CAN2_SendStd(uint16_t id, uint8_t *buf, uint8_t len);
//从 FIFO0 读一帧（轮询）
uint8_t CAN2_RecvStd(uint16_t *id, uint8_t *buf, uint8_t *len);
//滤波器：全部放行（调试用）
void CAN2_Filter_AcceptAll(void);
//打开 FIFO0 接收中断
void CAN2_IRQ_Enable(void);
//滤波器：32 位列表模式，放行两个 ID
void CAN2_Filter_List2(uint16_t id1, uint16_t id2);
//打印 TEC/REC/LEC/BOFF
void CAN2_PrintStatus(void);
