/**
 ****************************************************************************************************
 * @file        uds.c
 * @author      <aloneWalker>
 * @version     V1.0
 * @date        2026-11-01
 * @brief       ISO 14229 诊断服务实现
 ****************************************************************************************************
 */

#include "./BSP/CAN/uds.h"
#include "./BSP/CAN/isotp.h"
#include "./BSP/CAN/diag_cfg.h"
#include <string.h>
#include "./SYSTEM/delay/delay.h"
/**
 * @brief  获取系统毫秒时基
 * @note   HAL 库模板直接使用 HAL_GetTick()，返回上电以来的毫秒数。
 *         如果移植到标准库，把这里改成你的全局变量 g_ms_tick（在 1ms 定时器里自增）。
 *         使用宏定义是为了让上层代码不关心底层是用 HAL 还是标准库。
 */
#ifndef DIAG_TICK
#define DIAG_TICK()   HAL_GetTick()
#endif

// 当前诊断会话：默认、编程、扩展。初始为默认会话（SESS_DEFAULT）
static uint8_t s_session = SESS_DEFAULT;
// 安全访问状态：0 = 已锁定（未解锁），1 = 已解锁
static uint8_t s_unlocked = 0;
// 最后一次收到诊断请求的时间（毫秒），用于 S3 会话保持超时判断
static uint32_t s_last_req = 0;

/* 0x2E 写入的"指纹"，W7 刷写时由 uds_flash.c 读取 */
// 存储通过 0x2E 服务写入的指纹数据（用于刷写前的身份校验，最大 20 字节）
uint8_t g_fingerprint[20];
// 当前已写入的指纹实际长度
uint8_t g_fingerprint_len = 0;

/**
 * @brief  获取当前诊断会话
 * @return 当前会话状态（1=默认，2=编程，3=扩展）
 * @note   供其他模块查询当前处于哪个诊断会话中。
 */
uint8_t uds_session(void) {
    // 返回静态变量中保存的当前会话状态
    return s_session;
}

/**
 * @brief  获取当前安全解锁状态
 * @return 0 = 未解锁（锁定），1 = 已解锁
 * @note   供其他服务（如 0x2E 写数据、0x31 例程控制）判断是否允许执行敏感操作。
 */
uint8_t uds_unlocked(void) {
    // 返回静态变量中保存的解锁状态
    return s_unlocked;
}

/**
 * @brief  发送否定响应（NRC）
 * @param  sid: 被拒绝的服务 ID（如 0x22）
 * @param  nrc: 否定响应码（如 0x31 表示超出范围）
 * @note   否定响应的固定格式为：0x7F + 原服务ID + NRC。
 *         通过 ISO-TP 发送出去，由底层拆包/单帧发送。
 */
void uds_nrc(uint8_t sid, uint8_t nrc)
{
    // 定义 3 字节的数组，用于存放否定响应报文
    uint8_t r[3];
    // 第 0 字节：否定响应标识 0x7F
    r[0] = SID_NEGATIVE;
    // 第 1 字节：被拒绝的原始服务 ID
    r[1] = sid;
    // 第 2 字节：具体的否定响应码（NRC）
    r[2] = nrc;
    // 通过 ISO-TP 发送这 3 个字节（长度 3，会自动走单帧发送）
    isotp_send(r, 3);
}

/**
 * @brief  发送“响应挂起”否定响应（0x78）
 * @param  sid: 当前正在处理的服务 ID
 * @note   0x78 不是真正的错误，而是告诉诊断仪：“你的请求我收到了，
 *         但是处理需要时间（比如擦除 Flash），请等我真正处理完再回最终响应。”
 *         格式同样是 0x7F + 服务ID + 0x78。
 */
void uds_pending(uint8_t sid)
{
    // 定义 3 字节的数组，用于存放否定响应报文
    uint8_t r[3];
    // 第 0 字节：否定响应标识 0x7F
    r[0] = SID_NEGATIVE;
    // 第 1 字节：当前处理中的服务 ID
    r[1] = sid;
    // 第 2 字节：NRC 0x78，表示“响应挂起，正在处理”
    r[2] = NRC_PENDING;
    // 通过 ISO-TP 发送挂起响应
    isotp_send(r, 3);
}

/**
 * @brief  初始化 UDS 诊断服务状态
 * @note   上电时调用。将会话重置为默认会话，清除解锁状态，
 *         并刷新最后请求时间，防止刚上电就触发 S3 超时。
 */
void uds_init(void)
{
    // 默认进入默认会话（SESS_DEFAULT = 1）
    s_session = SESS_DEFAULT;
    // 默认处于锁定状态（安全访问未解锁）
    s_unlocked = 0;
    // 记录当前时间，作为 S3 计时器的起点
    s_last_req = DIAG_TICK();
}

/**
 * @brief  处理 0x10 诊断会话控制服务
 * @param  req: 请求数据缓冲区
 * @param  len: 请求数据长度
 * @note   作用：切换 ECU 的工作状态（默认会话、编程会话、扩展会话）。
 *         不同会话下，ECU 允许执行的诊断操作权限不同（比如编程会话才能刷程序）。
 */
static void svc_session(uint8_t *req, uint16_t len)
{
    // sub：存放提取出的子功能（会话类型）；r[6]：响应报文缓冲区
    uint8_t sub, r[6];

    // 检查长度：请求报文至少是 10 03（两个字节），如果少于2字节直接回“长度错误”
    if (len < 2) {uds_nrc(SID_SESSION, NRC_LENGTH); return; }

    // 提取子功能（req[1]），并屏蔽掉最高位（因为最高位可能是“抑制正响应”标志）
    sub = (uint8_t) (req[1] & SESS_SUB_MASK);

    // 检查会话类型是否合法：只能是 01（默认）、02（编程）、03（扩展）
    if (sub != SESS_DEFAULT && sub != SESS_PROGRAM && sub != SESS_EXTENDED) {
        uds_nrc(SID_SESSION, NRC_SUB_NOT_SUP);      // 不合法则回“子功能不支持”
        return;
    }

    // 验证通过，真正更新 ECU 内部的会话状态
    s_session = sub;

    // 关键安全机制：如果切回了“默认会话”，必须把安全解锁标志清零（重新上锁）
    if (sub == SESS_DEFAULT)s_unlocked = 0;         /* 回默认会话 → 上锁 */

    // 刷新最后请求时间，防止刚切完会话就触发了 S3（5秒）超时掉线
    s_last_req = DIAG_TICK();

    // ----- 开始组装肯定响应报文 (50 03 xx xx xx xx) -----
    // r[0]：正响应 SID = 请求 SID + 0x40（0x10 + 0x40 = 0x50）
    r[0] = (uint8_t) (SID_SESSION +UDS_POS_RESP_OFFSET);
    // r[1]：原样返回子功能（如 03）
    r[1] = sub;

    // r[2]、r[3]：P2 时间参数（默认 50ms），告诉诊断仪“以后我最多50ms就能给你回信”
    r[2] = (uint8_t) (UDS_P2_MS >> 8); r[3] = (uint8_t) (UDS_P2_MS &0xFF);
    // r[4]、r[5]：P2* 时间参数（默认 5000ms），告诉诊断仪“如果我回了 0x78 挂起，你最多等我5秒”
    r[4] = (uint8_t) (UDS_P2STAR_MS >> 8); r[5] = (uint8_t) (UDS_P2STAR_MS &0xFF);

    // 把这 6 个字节发出去
    isotp_send(r, 6);
}

/**
 * @brief  处理 0x3E 存在测试（心跳）服务
 * @param  req: 请求数据缓冲区
 * @param  len: 请求数据长度
 * @note   作用：诊断仪定期发这个，告诉 ECU“我还活着，别把我的会话退回到默认状态”。
 *         如果不发，5秒（S3）后 ECU 会自动退回默认会话。
 */
static void svc_tester_present(uint8_t *req, uint16_t len)
{
    // r[2]：响应报文缓冲区
    uint8_t r[2];
    // (void)len：告诉编译器“我知道 len 没用上，别报警告”
    (void)len;

    // 组装肯定响应：0x7E (0x3E + 0x40)
    r[0] = (uint8_t) (SID_TESTER_PRES +UDS_POS_RESP_OFFSET);
    // 响应固定返回 0x00
    r[1] = 0;

    // 刷新最后请求时间，给 S3 计时器续命
    s_last_req = DIAG_TICK();

    // 【高级特性】检查“抑制正响应位”（第 7 位，0x80）
    // 如果诊断仪设了这位，说明“我知道你收到了，不用回我消息了，节约总线带宽”
    // 所以如果 (req[1] & 0x80) == 0，才真正发送响应报文
    if ((req[1] & UDS_SUPPRESS_POS) == 0)isotp_send(r, 2); /* suppressPosRsp 位 */
}

/**
 * @brief  处理 0x11 ECU 复位服务
 * @param  req: 请求数据缓冲区
 * @param  len: 请求数据长度
 * @note   作用：让 ECU 重启（比如刷写完程序后，需要重启生效）。
 */
static void svc_ecu_reset(uint8_t *req, uint16_t len)
{
    // sub：复位类型；r[2]：响应报文缓冲区
    uint8_t sub, r[2];

    // 长度必须 ≥ 2（如 11 01）
    if (len < 2) {uds_nrc(SID_ECU_RESET, NRC_LENGTH); return; }

    // 提取复位类型（屏蔽最高位）
    sub = (uint8_t) (req[1] & SESS_SUB_MASK);

    // 校验复位类型：只能是 01（硬复位）或 03（软复位）
    if (sub != ECU_RESET_HARD && sub != ECU_RESET_SOFT) {
        uds_nrc(SID_ECU_RESET, NRC_SUB_NOT_SUP);
        return;
    }

    // 组装肯定响应：51 01 (0x11 + 0x40 = 0x51)
    r[0] = (uint8_t) (SID_ECU_RESET +UDS_POS_RESP_OFFSET); r[1] = sub;

    // 先发送肯定响应，告诉诊断仪“我收到重启指令了”
    isotp_send(r, 2);

    // 重要延时：因为发送 CAN 报文需要时间，如果刚调完发送函数立刻重启，报文可能发不出去
    // 等 50ms 确保报文完整发到了总线上，再执行重启
    delay_ms(50);

    // 调用底层硬件接口，执行芯片复位（重启）
    HAL_NVIC_SystemReset();
}

/* ============================================================
 * 0x22 ReadDataByIdentifier
 * 返回响应长度；0 = 该 DID 不支持
 * ============================================================ */
/**
 * @brief  处理 0x22 读取数据标识符（Read Data By Identifier）服务
 * @param  req: 请求缓冲区（如 22 F1 90，req[1]是高字节F1，req[2]是低字节90）
 * @param  len: 请求长度
 * @param  out: 响应缓冲区（调用者提供的大数组，用于拼装应答）
 * @return 返回需要发送的字节数；返回 0 表示出错（已自动回了否定响应）
 * @note   作用：读取 ECU 内部的数据（如版本号、VIN码、序列号）。
 *         读取的内容长度不一，短的几字节（单帧），长的如VIN码有20字节（自动走多帧）。
 */
static uint16_t svc_read_did(uint8_t *req, uint16_t len, uint8_t *out)
{
    // did：存放拼出来的 16 位数据标识符；n：记录响应报文的长度
    uint16_t did, n = 0;

    // 长度至少 3：SID + DID 高字节 + DID 低字节（如 22 F1 90 是3个字节）
    if (len < 3)
    {
        uds_nrc(SID_READ_DID, NRC_LENGTH);          // 长度不够，直接回“长度错误”并退出
        return 0;
    }

    // 拼出 16 位 DID：把 req[1]（高字节）左移 8 位，再和 req[2]（低字节）做或运算
    // 例如 req[1]=F1, req[2]=90，拼出来就是 0xF190
    did = (uint16_t) (((uint16_t)req[1] << 8) | req[2]);

    // 不管读什么，正响应的前三字节格式是固定的：
    // 第 0 字节：正响应 SID（0x22 + 0x40 = 0x62）
    out[0] = (uint8_t) (SID_READ_DID +UDS_POS_RESP_OFFSET);
    // 第 1 字节：原样返回 DID 高字节
    out[1] = req[1];
    // 第 2 字节：原样返回 DID 低字节
    out[2] = req[2];
    // 长度 n 初始化为 3（因为已经填了 3 个字节了）
    n = 3;

    // 根据不同的 DID 读取不同的数据，并追加到 out 缓冲区后面
    switch (did)
    {
        case DID_SESSION:                           // F186：当前会话
        out[n++] = s_session;                       // 把当前会话状态（1字节）放入缓冲区，长度 n 加 1
        break;

        case DID_VIN:                               // F190：VIN，17 字节 → 响应 20 字节，走多帧
        memcpy(&out[n], VIN_STR, 17);               // 把 VIN 字符串（17字节）拷到缓冲区
        n = (uint16_t) (n + 17);                    // 长度 n 加上 17（总共 3 + 17 = 20 字节）
        break;

        case DID_APP_VERSION:                       // F195：软件版本字符串
        {
            const char *v = APP_VERSION;            // 拿到版本号的字符串指针
            uint8_t vl = (uint8_t)strlen(v);        // 计算字符串长度（动态）
            memcpy(&out[n], v, vl);                 // 把版本号拷贝到缓冲区
            n = (uint16_t) (n + vl);                // 长度 n 加上版本号字符串的长度
        }
        break;

        case DID_ECU_SN:                            // F18C：ECU 序列号 = UID 低 4 字节
        {
            uint32_t uid = HAL_GetUIDw0();          // 读取芯片唯一 ID 的低 32 位（4字节）
            // 把 32 位 UID 拆成 4 个字节，依次放入缓冲区（大端序：高位在前）
            out[n++] = (uint8_t) (uid >> 24);       // 最高 8 位
            out[n++] = (uint8_t) (uid >> 16);       // 次高 8 位
            out[n++] = (uint8_t) (uid >> 8);        // 次低 8 位
            out[n++] = (uint8_t)uid;                // 最低 8 位
        }
        break;

        case DID_FINGERPRINT:                       // F199：D11 写进来的指纹，读回验证
        {
            // 把之前 0x2E 写入到 g_fingerprint 的数据拷出来
            memcpy(&out[n], g_fingerprint, g_fingerprint_len);
            n = (uint16_t) (n + g_fingerprint_len); // 长度 n 加上指纹的实际长度
        }
        break;

        default:
        // 如果请求的 DID 不在支持列表里，回“超出范围”否定响应，并返回 0 终止发送
        uds_nrc(SID_READ_DID, NRC_RANGE);
        return 0;
    }

    // 返回最终拼好的响应报文总长度（调用者会用这个长度去发 isotp_send）
    return n;
}

/**
 * @brief  处理 0x2E 写入数据标识符（Write Data By Identifier）服务
 * @param  req: 请求缓冲区（如 2E F1 99 xx xx）
 * @param  len: 请求长度
 * @note   作用：向 ECU 写入数据（如写入指纹、写配置参数）。
 *         出于安全考虑，规定默认会话下禁止写入。
 */
static void svc_write_did(uint8_t *req, uint16_t len)
{
    // did：待写入的数据标识符；r[3]：肯定响应缓冲区
    uint16_t did;
    uint8_t r[3];

    // 长度至少 4：SID + DID 高 + DID 低 + 至少 1 字节数据
    if (len < 4) {uds_nrc(SID_WRITE_DID, NRC_LENGTH); return; }

    // 拼出 16 位 DID
    did = (uint16_t) (((uint16_t)req[1] << 8) | req[2]);

    // 【安全门控】如果当前是默认会话（SESS_DEFAULT = 1），禁止写入
    // 必须先用 0x10 切到扩展/编程会话，才允许修改重要数据
    if (s_session == SESS_DEFAULT) {uds_nrc(SID_WRITE_DID, NRC_COND); return; }

    // 判断要写的是不是“指纹”（F199）
    if (did == DID_FINGERPRINT) {                   /* 指纹 */
        // 算出要写入的数据长度：总长度减去前 3 个字节（SID + DID）
        uint16_t dl = (uint16_t) (len - 3);

        // 防越界检查：如果写入的数据超过指纹缓冲区的最大容量（20字节），拒绝
        if (dl > sizeof (g_fingerprint)) {uds_nrc(SID_WRITE_DID, NRC_RANGE); return; }

        // 把请求中的数据部分（从 req[3] 开始）拷到全局指纹缓冲区
        memcpy(g_fingerprint, &req[3], dl);
        // 记录实际写入的指纹长度
        g_fingerprint_len = (uint8_t)dl;
    }
    else {
        // 如果要写的 DID 不是指纹（暂不支持写其他），回“超出范围”
        uds_nrc(SID_WRITE_DID, NRC_RANGE);
        return;
    }

    // 组装肯定响应：6E F1 99（0x2E + 0x40 = 0x6E，后跟原样返回的 DID）
    r[0] = (uint8_t) (SID_WRITE_DID +UDS_POS_RESP_OFFSET);
    r[1] = req[1];
    r[2] = req[2];

    // 发送这 3 个字节的肯定响应
    isotp_send(r, 3);
}

/**
 * @brief  处理一条完整的 UDS 诊断请求（核心分发器/总机转接员）
 * @param  req: 请求数据缓冲区（已经由 ISO-TP 拼好的完整数据）
 * @param  len: 请求数据的长度（字节数）
 * @note   这是整个 UDS 服务的总入口。主循环 uds_poll() 发现收到了一包完整数据，
 *         就会把数据扔进这个函数。它负责把活儿派发给具体的处理函数。
 */
void uds_request(uint8_t *req, uint16_t len)
{
    // 提取本次请求的服务 ID（SID），比如 0x10、0x22、0x2E
    uint8_t sid;

    // 通用响应缓冲区，大小 64 字节。
    // 为什么要弄个 64 字节？因为像 0x22 读数据（比如读 VIN 码），
    // 响应不是固定的两三个字节，而是可能长达 20 多个字节，
    // 必须要有一块空地，让业务部门（比如 svc_read_did）把数据先拼好放这里。
    uint8_t resp[64];                               /* 留给 0x22 的应答缓冲 */

    // 用于接收某些服务函数返回的应答长度。
    // 比如 0x22 读到了 20 个字节的数据，它处理完会告诉总机：“我拼好了，长度是 20”。
    uint16_t n;

    // 防御性检查：长度至少为 1（否则连 SID 都没有，无法处理）
    // 如果连个字节都没收到，那就别处理了，防呆设计。
    if (len < 1)return;

    // 获取请求的第一个字节作为服务 ID
    sid = req[0];

    // 任何合法的请求都算作“诊断仪还在活动”，刷新最后请求时间。
    // 用于主循环里的 S3（5秒会话保持）超时计时，防止会话因为无操作而掉线。
    s_last_req = DIAG_TICK();                       /* 任何请求都算"活动"，喂给 S3 计时 */

    // 根据服务 ID 进行分发（开始转接总机电话）
    switch (sid)
    {
        // 诊断仪发来了 0x10（会话控制），转接给 svc_session 部门。
        // 这个部门内部自己会把肯定/否定响应发出去，所以这里不需要 isotp_send。
        case SID_SESSION:
        svc_session(req, len);
        break;

        // 诊断仪发来了 0x11（ECU复位），转接给 svc_ecu_reset。
        // 注意：它发完响应后，芯片会直接重启（SystemReset）。
        case SID_ECU_RESET:
        svc_ecu_reset(req, len);
        break;

        // 诊断仪发来了 0x3E（心跳保持），转接给 svc_tester_present。
        // 这个部门处理很快，就是为了不让会话掉线。
        case SID_TESTER_PRES:
        svc_tester_present(req, len);
        break;

        // 诊断仪发来了 0x22（读数据），转接给 svc_read_did。
        // 这个稍微特殊：它的响应长度不固定（比如 VIN 20 字节），走 resp 缓冲。
        case SID_READ_DID:
        n = svc_read_did(req, len, resp);           // 业务部门干活，把数据填进 resp，返回长度 n
        if (n > 0)isotp_send(resp, n);              // 如果 n > 0（说明没出错），总机帮忙把 resp 发出去
        break;

        // 诊断仪发来了 0x2E（写数据），转接给 svc_write_did。
        // 内部也是自己发包。
        case SID_WRITE_DID:
        svc_write_did(req, len);
        break;

        // 如果诊断仪发了个 0x99，在当前的 switch 列表里找不到对应的 case。
        // 说明这个服务还没实现或者不支持（比如还没写 0x27、0x31）。
        // 直接回复“服务不支持”的否定响应（7F SID 11）。
        default:
        uds_nrc(sid, NRC_SID_NOT_SUP);
        break;
    }
}
