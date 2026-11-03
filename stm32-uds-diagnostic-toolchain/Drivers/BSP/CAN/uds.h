/**
 ****************************************************************************************************
 * @file        uds.h
 * @author      <aloneWalker>
 * @version     V1.0
 * @date        2026-11-01
 * @brief       ISO 14229 诊断服务（ECU 侧）
 ****************************************************************************************************
 */

#ifndef __CAN_UDS_H
#define __CAN_UDS_H

#include "./SYSTEM/sys/sys.h"

//* --------------------------- 服务号 -------------------------------- */
// 0x10：会话控制。比如从“默认模式”切换到“扩展模式”（解锁更多高级权限）
#define SID_SESSION             0x10u
// 0x11：ECU复位。相当于让单片机重启（刷完程序后必须重启生效）
#define SID_ECU_RESET           0x11u
// 0x22：读数据。比如读取车架号(VIN)、读取当前软件版本号
#define SID_READ_DID            0x22u
// 0x27：安全访问。请求种子和发送密钥（防止阿猫阿狗都能刷你的车）
#define SID_SECURITY            0x27u
// 0x2E：写数据。比如写入指纹信息、写入新的配置参数
#define SID_WRITE_DID           0x2Eu
// 0x31：例程控制。比如让ECU执行“擦除Flash”、“校验APP区”的动作
#define SID_ROUTINE             0x31u
// 0x34：请求下载。告诉ECU“我要开始传新程序了，程序总大小是多少”
#define SID_DL_REQUEST          0x34u
// 0x36：传输数据。一块一块地把新程序代码传给ECU
#define SID_TRANSFER            0x36u
// 0x37：退出传输。程序传完了，告诉ECU“传完了，你可以关闭接收通道了”
#define SID_TRANSFER_EXIT       0x37u
// 0x3E：存在测试。也就是“心跳包”，告诉ECU“我还在，别把我的连接断开”
#define SID_TESTER_PRES         0x3Eu
// 0x7F：否定响应标志。只要ECU拒绝了你的请求，回复的第一字节肯定是 0x7F
#define SID_NEGATIVE            0x7Fu

/* 正响应 = 请求 SID + 0x40 */
// 国际规定：如果请求是 0x10，ECU同意后，回复的首字节就是 0x10 + 0x40 = 0x50。
// 看到 0x50，就知道 0x10 的请求成功了。
#define UDS_POS_RESP_OFFSET     0x40u

/* -------------------------- 否定响应码 ------------------------------ */
// NRC 全称 Negative Response Code，就是“拒绝的理由”
// 0x11：不认识这个服务（不支持的服务号）
#define NRC_SID_NOT_SUP         0x11u
// 0x12：服务号认识，但不认识你发来的子功能（比如发 10 09，但09不是合法会话）
#define NRC_SUB_NOT_SUP         0x12u
// 0x13：长度不对（比如本该发3个字节，你只发了2个）
#define NRC_LENGTH              0x13u
// 0x22：条件不满足（比如在默认会话下，你想直接写数据，ECU拒绝）
#define NRC_COND                0x22u
// 0x24：顺序错误（比如你没请求下载，直接传数据）
#define NRC_SEQUENCE            0x24u
// 0x31：超出范围（比如你想读一个根本不存在的车架号ID）
#define NRC_RANGE               0x31u
// 0x33：安全拒绝（你还没解锁安全权限，就想干敏感操作）
#define NRC_SEC_DENIED          0x33u
// 0x35：密钥错误（你算出来的密码不对）
#define NRC_KEY_INVALID         0x35u
// 0x36：尝试次数超限（密码连续输错3次，把你拉黑一会儿）
#define NRC_ATTEMPTS            0x36u
// 0x72：一般性失败（没有特定原因，就是失败了，比如擦除Flash出错了）
#define NRC_GENERAL_FAIL        0x72u
// 0x73：块序号错误（传数据包时，包的序号乱了）
#define NRC_BLOCK_SEQ           0x73u
// 0x78：请求正确，但正在处理中（不是真正的错误，让诊断仪再等等）
#define NRC_PENDING             0x78u

/* ---------------------------- 会话 --------------------------------- */
// 会话类型，相当于你的“操作权限级别”
// 0x01：默认会话（最基础的权限，一开机默认就在这个状态）
#define SESS_DEFAULT            0x01u
// 0x02：编程会话（刷写程序的专用权限，必须切到这里才能刷机）
#define SESS_PROGRAM            0x02u
// 0x03：扩展会话（高级诊断权限，比如写入数据、执行特殊测试）
#define SESS_EXTENDED           0x03u
// 掩码：诊断仪发来的会话命令最高位可能带“抑制正响应”标志（0x80），
// 我们只看低 7 位（0x7F），也就是真正的会话类型 01/02/03。
#define SESS_SUB_MASK           0x7Fu

/* -------------------------- 子功能位 ------------------------------- */
// 置 1 表示不回复正响应。比如发 3E 80，ECU 收到心跳后就不回 7E 00 了，节省总线资源
#define UDS_SUPPRESS_POS        0x80u

/* ------------------------- 0x11 复位类型 ---------------------------- */
// 0x01：硬复位（完全重启芯片）
#define ECU_RESET_HARD          0x01u
// 0x03：软复位（只重启软件逻辑，硬件不断电）
#define ECU_RESET_SOFT          0x03u

/* ------------------------ 安全访问子功能 ---------------------------- */
// 0x01：请求种子（第一步，诊断仪说：给我个随机数当暗号）
#define SEC_SEED_REQ            0x01u
// 0x02：发送密钥（第二步，诊断仪把计算好的密码发给ECU对答案）
#define SEC_KEY_SEND            0x02u

/* -------------------------- 例程控制 ------------------------------- */
// 动作类型：启动某个例程
#define ROUTINE_START           0x01u
// 动作类型：停止某个例程
#define ROUTINE_STOP            0x02u
// 动作类型：查询例程的结果
#define ROUTINE_RESULT_REQ      0x03u
// 例程ID：擦除APP区（刷机前，先要把旧程序抹掉）
#define ROUTINE_ERASE_APP       0xFF00u
// 例程ID：校验APP区（刷完机后，算一下CRC看看代码有没有传坏）
#define ROUTINE_CHECK_APP       0xFF01u

/* --------------------------- 数据标识 ------------------------------ */
// 数据标识ID（DID）。标准规定的通用ID：
// 0xF190：读 VIN 码（车架号）
#define DID_VIN                 0xF190u
// 0xF195：读 软件版本号
#define DID_APP_VERSION         0xF195u
// 0xF18C：读 ECU 序列号
#define DID_ECU_SN              0xF18Cu
// 0xF186：读 当前会话状态
#define DID_SESSION             0xF186u
// 0xF199：读 指纹（这是自己定的，用来验证是否经过了刷机流程）
#define DID_FINGERPRINT         0xF199u

/* --------------------------- 指纹长度 ------------------------------ */
// 自定义的指纹最大长度限制为20字节
#define FP_MAX_LEN              20

/* ------------------------ 0x34 传输格式 ----------------------------- */
// 传输格式里的“长度格式”：0x20 表示后面用 2 个字节来存总长度
#define DL_LENFMT_2BYTE         0x20u
// 提取 4 位（比如 0x44 的高4位是4，低4位也是4）
#define DL_NIBBLE_MASK          0x0Fu
// 把数值右移 4 位（用来提取高4位）
#define DL_NIBBLE_SHIFT         4
// 一次下载请求，最少也得有 8 个字节吧（不能太小）
#define DL_MIN_SIZE             8

/* 函数声明 */
//初始化会话/解锁/计时状态
void uds_init(void);
//处理一条完整请求
void uds_request(uint8_t *req, uint16_t len);
//主循环调用：分发请求 + S3 超时
void uds_poll(void);
//回复否定响应
void uds_nrc(uint8_t sid, uint8_t nrc);
//回复 0x78 响应挂起
void uds_pending(uint8_t sid);
//读当前会话
uint8_t uds_session(void);
//读是否已解锁
uint8_t uds_unlocked(void);

#endif
