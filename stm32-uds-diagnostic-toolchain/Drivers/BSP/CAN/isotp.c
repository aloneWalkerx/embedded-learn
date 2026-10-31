/**
 ****************************************************************************************************
 * @file        isotp.c
 * @author      <aloneWalker>
 * @version     V1.0
 * @date        2026-10-27
 * @brief       ISO 15765-2 传输层
 ****************************************************************************************************
 */
#include "./BSP/CAN/isotp.h"
#include "./BSP/CAN/diag_cfg.h"
#include "./BSP/CAN/can2.h"
#include <string.h>
#include "./SYSTEM/delay/delay.h"

// ISO-TP 接收缓冲区，最大 4095 字节（ISO 15765-2 上限）
uint8_t g_iso_rx_buf[ISO_MAX_LEN];
// 当前接收缓冲区里的数据长度
uint16_t g_iso_rx_len = 0;
// 有一包完整请求待处理标志：中断里只置 1，主循环 uds_poll() 处理完清 0，中断里只置标志，回包放到主循环
volatile uint8_t g_iso_req_pending = 0;

// 本 ECU 接收诊断请求的 CAN ID（默认 0x7E0）
static uint16_t s_rx_id = ISO_RX_ID;
// 本 ECU 发送诊断响应的 CAN ID（默认 0x7E8）
static uint16_t s_tx_id = ISO_TX_ID;

/* ---- 接收侧 ---- */
// 接收状态机：0=空闲，1=正在收多帧（FF 已收，等 CF）
static uint8_t s_rx_active = 0;
// 多帧总长度（FF 里带的 12 位长度）
static uint16_t s_rx_total = 0;
// 多帧已收到的字节数
static uint16_t s_rx_got = 0;
// 期望的下一个连续帧序号（1~15，0 回绕）
static uint8_t s_rx_sn = 0;

/* ---- 发送侧 ---- */
// 发送忙标志：1=正在发多帧（等 FC 或发 CF 中）
static uint8_t s_tx_busy = 0;
// 当前发送的连续帧序号
static uint8_t s_tx_sn = 0;
// 是否收到对端 FC：中断里置 1，iso_wait_fc 轮询到后清 0
static volatile uint8_t s_fc_seen = 0;

/**
 * @brief  初始化 ISO-TP 传输层
 * @param  rx_id: 接收诊断请求的 CAN ID（如 0x7E0）
 * @param  tx_id: 发送诊断响应的 CAN ID（如 0x7E8）
 * @note   设置收发 ID，清空接收状态机、发送忙标志、FC 标志和接收长度。
 *         上电后调用一次即可。
 */
void isotp_init(uint16_t rx_id, uint16_t tx_id)
{
    // 记录接收 ID
    s_rx_id = rx_id;
    // 记录发送 ID
    s_tx_id = tx_id;
    // 清接收状态机
    s_rx_active = 0;
    // 清发送忙标志
    s_tx_busy = 0;
    // 清 FC 标志
    s_fc_seen = 0;
    // 清接收长度
    g_iso_rx_len = 0;
}

/**
 * @brief  复位 ISO-TP 状态机
 * @note   异常恢复时调用（如超时、序号错乱后），清接收状态机、发送忙标志、FC 标志。
 *         不改变收发 ID，也不清空缓冲区内容。
 */
void isotp_reset(void)
{
    // 清接收状态机
    s_rx_active = 0;
    // 清发送忙标志
    s_tx_busy = 0;
    // 清 FC 标志
    s_fc_seen = 0;
}

/**
 * @brief  查询是否正在发送多帧
 * @return 1 = 忙（正在等 FC 或发 CF 中）；0 = 空闲
 * @note   主循环在发下一包前可调用它，避免与上一包冲突。
 */
uint8_t isotp_is_busy(void) {
    return s_tx_busy;
}

/**
 * @brief  发送流控帧（FC）
 * @param  fs: 流控状态，0 = CTS（继续发）/ 1 = WAIT（等待）/ 2 = OVFLW（溢出）
 * @note   我方作为接收方，收到 FF 后回 FC 告诉对端能否继续发。
 *         帧格式：字节 0 = PCI | fs，字节 1 = BS（0 不限块），字节 2 = STmin（5ms），
 *         字节 3~7 填 0。通过 CAN2 以 s_tx_id 发出。
 */
static void iso_send_fc(uint8_t fs)
{
    // 临时缓冲，一帧 CAN 数据 8 字节
    uint8_t f[8];
    // 字节 0：PCI，高 4 位 = FC(0x30)，低 4 位 = 流控状态 fs
    f[0] = (uint8_t) (ISOTP_PCI_FC | (fs & ISOTP_LEN_MASK));
    // 字节 1：BS（Block Size），0 表示不限块
    f[1] = ISO_BS_ADVERT;
    // 字节 2：STmin（最小间隔），5ms
    f[2] = ISO_STMIN_MS;
    // 字节 3~7：填充 0
    f[3] = 0; f[4] = 0; f[5] = 0; f[6] = 0; f[7] = 0;
    // 通过 CAN2 发出，ID = s_tx_id
    CAN2_SendStd(s_tx_id, f, 8);
}

/**
 * @brief  处理收到的一帧 CAN 数据（由 CAN 中断回调调用）
 * @param  id:   收到的帧 ID
 * @param  data: 数据指针（首字节是 PCI）
 * @param  len:  数据长度（DLC，1~8）
 * @note   按首字节高 4 位判断帧类型：
 *         - SF（单帧）：拷贝数据到 g_iso_rx_buf，置 g_iso_req_pending
 *         - FF（首帧）：解析总长度，拷贝前 6 字节，回 FC（CTS），进入收多帧状态
 *         - CF（连续帧）：校验序号，拼接到 g_iso_rx_buf，收满置 g_iso_req_pending
 *         - FC（流控帧）：置 s_fc_seen，供 isotp_send 的 iso_wait_fc 轮询
 *         只处理 ID == s_rx_id 的帧，其他 ID 直接丢弃。不在 ISR 里回包。
 */
void isotp_rx_frame(uint16_t id, uint8_t *data, uint8_t len)
{
    // PCI：首字节高 4 位，表示帧类型
    uint8_t pci;

    // ID 不匹配或数据长度 < 1，直接丢弃
    if (id != s_rx_id || len < 1)return;
    // 提取首字节高 4 位，判断帧类型
    pci = (uint8_t) (data[0] & ISOTP_PCI_MASK);

    // 根据帧类型分派处理
    switch (pci)
    {
        // ---- 单帧 ----
        case ISOTP_PCI_SF:
        {
            // 低 4 位 = 数据长度（0~7）
            uint8_t n = (uint8_t) (data[0] & ISOTP_LEN_MASK);
            // 长度非法（0、>7、或超过实际数据）则丢弃
            if (n == 0 || n > 7 || n > (len - 1))return;
            // 把数据从 data[1] 开始拷贝 n 字节到接收缓冲
            memcpy(g_iso_rx_buf, &data[1], n);
            // 记录接收长度
            g_iso_rx_len = n;
            // 单帧结束，清接收状态机
            s_rx_active = 0;
            // 置"有一包完整请求"标志，交给主循环 uds_poll() 处理,不在 ISR 里回包，交给 uds_poll()
            g_iso_req_pending = 1;
            break;
        }
        // ---- 首帧 ----
        case ISOTP_PCI_FF:
        {
            // 12 位总长度：高 4 位在 data[0]的 低 4 位，低 8 位在 data[1]
            uint16_t total = (uint16_t) (((uint16_t) (data[0] & ISOTP_LEN_MASK) << 8) | data[1]);
            // FF 带的数据字节数 = 总长度 - 2（PCI 占 1 字节，长度占 1 字节）
            uint8_t n = (uint8_t) (len - 2);
            // 总长度非法（<=7 应该用 SF、>4095 超上限）则丢弃
            if (total <= 7 || total > ISO_MAX_LEN)return;
            // 把 data[2] 开始的 n 字节拷到接收缓冲
            memcpy(g_iso_rx_buf, &data[2], n);
            // 记录总长度
            s_rx_total = total;
            // 记录已收字节数
            s_rx_got = n;
            // 期望的下一个 CF 序号 = 1
            s_rx_sn = 1;
            // 置接收状态机为"收多帧中"
            s_rx_active = 1;
            // 回 FC，告诉对端可以继续发（CTS = 0）
            iso_send_fc(0);                         /* CTS */
            break;
        }
        // ---- 连续帧 ----
        case ISOTP_PCI_CF:
        {
            // 低 4 位 = 连续帧序号
            uint8_t sn = (uint8_t) (data[0] & ISOTP_LEN_MASK);
            // 本帧要拷贝的数据字节数
            uint8_t n;
            // 没在收多帧状态，丢弃
            if (!s_rx_active)return;
            // 序号不匹配，丢整包（清状态机）
            if (sn != s_rx_sn) {s_rx_active = 0; return; } /* 序号错 → 丢整包 */

            // CF 带的数据字节数 = 总长度 - 1（PCI 占 1 字节）
            n = (uint8_t) (len - 1);
            // 如果剩余空间不够，截断到剩余长度
            if ((uint16_t) (s_rx_got + n) > s_rx_total)n = (uint8_t) (s_rx_total - s_rx_got);
            // 把 data[1] 开始的 n 字节拷到接收缓冲的对应位置
            memcpy(&g_iso_rx_buf[s_rx_got], &data[1], n);
            // 更新已收字节数
            s_rx_got += n;
            // 序号 +1，回绕到 0~15
            s_rx_sn = (uint8_t) ((s_rx_sn + 1) & ISOTP_LEN_MASK);

            // 收满了整包
            if (s_rx_got >= s_rx_total) {
                // 清接收状态机
                s_rx_active = 0;
                // 记录最终长度
                g_iso_rx_len = s_rx_total;
                // 置"有一包完整请求"标志，交给主循环 uds_poll() 处理
                g_iso_req_pending = 1;              /* 不在 ISR 里回包，交给 uds_poll() */
            }
            break;
        }
        // ---- 流控帧（发送侧关心） ----
        case ISOTP_PCI_FC:
        // 置 FC 标志，iso_wait_fc 会轮询到它
        s_fc_seen = 1;
        break;
    }
}

/**
 * @brief  等待对端回 FC（流控帧）
 * @param  timeout_ms: 超时时间（毫秒）
 * @return ISOTP_OK = 收到 FC；ISOTP_ERR = 超时
 * @note   阻塞式轮询，每 1ms 检查一次 s_fc_seen。
 *         收到 FC 后清 s_fc_seen，返回成功；超时返回失败。
 *         s_fc_seen 由 isotp_rx_frame() 在中断里置位。
 */
static uint8_t iso_wait_fc(uint16_t timeout_ms)
{
    // 计时变量（毫秒）
    uint16_t t;
    // 轮询 timeout_ms 次，每次 1ms
    for (t = 0; t < timeout_ms; t++) {
        // 收到 FC 了，清标志并返回成功
        if (s_fc_seen) {s_fc_seen = 0; return ISOTP_OK; }
        // 等 1ms
        delay_ms(1);
    }
    // 超时，返回失败
    return ISOTP_ERR;
}

/**
 * @brief  发送一包数据（阻塞式，含 FC 等待）
 * @param  buf: 数据缓冲区指针
 * @param  len: 数据长度（1~4095）
 * @return ISOTP_OK = 成功；ISOTP_ERR = 失败（长度非法、发送失败、等 FC 超时）
 * @note   长度 <= 7 字节：用单帧（SF）发出；
 *         长度 > 7 字节：先发首帧（FF），等对端 FC，再逐帧发连续帧（CF），
 *         每帧之间按 ISO_STMIN_MS（5ms）延时。
 *         发送期间 s_tx_busy = 1，发送结束清 0。
 */
uint8_t isotp_send(uint8_t *buf, uint16_t len)
{
    // 临时缓冲，一帧 CAN 数据 8 字节
    uint8_t f[8];
    // n：本帧数据字节数；i：循环变量
    uint8_t n, i;
    // 已发送偏移
    uint16_t off = 0;

    // 长度非法（0 或 >4095）直接返回失败
    if (len == 0 || len > ISO_MAX_LEN)return ISOTP_ERR;

    /* 短报文：单帧 */
    // 长度 <= 7，用单帧发
    if (len <= 7) {
        // 字节 0：PCI，低 4 位 = 长度（高 4 位 = 0 表示 SF）
        f[0] = (uint8_t)len;
        // 拷贝数据到 f[1..len]
        for (i = 0; i < len; i++)f[1 + i] = buf[i];
        // 剩余字节填填充字节 0xCC
        for (i = (uint8_t) (len + 1); i < 8; i++)f[i] = ISOTP_PAD_BYTE;
        // 发出去，成功返回 OK，失败返回 ERR
        return CAN2_SendStd(s_tx_id, f, 8) ? ISOTP_ERR : ISOTP_OK;
    }

    /* 首帧 */
    // 字节 0：PCI，高 4 位 = FF(0x10)，低 4 位 = 总长度的高 4 位
    f[0] = (uint8_t) (ISOTP_PCI_FF | ((len >> 8) & ISOTP_LEN_MASK));
    // 字节 1：总长度的低 8 位
    f[1] = (uint8_t) (len & 0xFFU);
    // 字节 2~7：前 6 字节数据
    for (i = 0; i < 6; i++)f[2 + i] = buf[i];
    // 已发送偏移 = 6
    off = 6;
    // 第一个 CF 序号 = 1
    s_tx_sn = 1;
    // 置发送忙标志
    s_tx_busy = 1;
    // 清 FC 标志（等对端回 FC）
    s_fc_seen = 0;

    // 发送首帧，失败则清忙标志并返回 ERR
    if (CAN2_SendStd(s_tx_id, f, 8)) {s_tx_busy = 0; return ISOTP_ERR; }
    // 等对端 FC，超时则清忙标志并返回 ERR
    if (iso_wait_fc(ISO_FC_TIMEOUT)) {s_tx_busy = 0; return ISOTP_ERR; }

    /* 连续帧 */
    // 一直发到发完
    while (off < len)
    {
        // 本帧要发的字节数 = 剩余长度
        n = (uint8_t) (len - off);
        // 最多 7 字节（CF 数据段容量）
        if (n > 7)n = 7;
        // 字节 0：PCI，高 4 位 = CF(0x20)，低 4 位 = 序号
        f[0] = (uint8_t) (ISOTP_PCI_CF | (s_tx_sn & ISOTP_LEN_MASK));
        // 拷贝 n 字节数据到 f[1..n]
        for (i = 0; i < n; i++)f[1 + i] = buf[off + i];
        // 剩余字节填填充字节 0xCC
        for (i = (uint8_t) (n + 1); i < 8; i++)f[i] = ISOTP_PAD_BYTE;

        // 发送 CF，失败则清忙标志并返回 ERR
        if (CAN2_SendStd(s_tx_id, f, 8)) {s_tx_busy = 0; return ISOTP_ERR; }
        // 更新已发送偏移
        off += n;
        // 序号 +1，回绕到 0~15
        s_tx_sn = (uint8_t) ((s_tx_sn + 1) & ISOTP_LEN_MASK);
        // 按 STmin 延时 5ms，避免发太快
        delay_ms(ISO_STMIN_MS);
    }

    // 发完，清忙标志
    s_tx_busy = 0;
    // 返回成功
    return ISOTP_OK;
}
