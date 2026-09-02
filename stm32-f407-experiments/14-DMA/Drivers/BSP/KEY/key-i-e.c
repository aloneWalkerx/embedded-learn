#include "./BSP/KEY/key-i.h"
#include "./BSP/KEY/key-e.h"
#include "./BSP/KEY/key-i-e.h"
#include "./SYSTEM/delay/delay.h"


uint8_t key_i_e_scan(uint8_t mode)
{
    static uint8_t key_release = 1;   // 静态变量，记录上次按键是否已释放（1=已释放）
    uint8_t key_value = I_E_NONE_PRESS;   // 默认无按键

    // 【模式控制】如果 mode 不为 0（即 mode=1），强制认为按键已释放 → 开启连续触发
    if (mode != 0) {
        key_release = 1;
    }

    // 【主检测条件】只有在“已释放”状态，且任一按键被按下时，才进入处理
    if ((key_release == 1) && ((WKUP_STATUS == 1) || (KEY0_STATUS == 1) || (KEY_R_STATUS == 0) || (KEY_Y_STATUS == 0) || (KEY_G_STATUS == 0))) {
        delay_ms(10);                // 软件消抖（延时约10ms）
        key_release = 0;             // 标记“已按下”，避免本次重复触发

        // 判断具体是哪个按键按下了
        
        
        if (KEY_R_STATUS == 0) {
            key_value = I_E_KEY_R_PRESS;
        }
        
        if (KEY_Y_STATUS == 0) {
            key_value = I_E_KEY_Y_PRESS;  
        }
        
        if (KEY_G_STATUS == 0) {
            key_value = I_E_KEY_G_PRESS;  
        }
        
        
        if (KEY0_STATUS == 1) {
            key_value = I_E_KEY0_PRESS;
        }
        
        if (WKUP_STATUS == 1) {
            key_value = I_E_WKUP_PRESS; // 如果五个同时按下，WKUP 会覆盖 KEY0,KEY_R，KEY_Y,KEY_G（即WKUP优先于KEY0,KEY_R,KEY_Y,KEY_G）
        }
    }
    // 【释放检测】如果两个按键都处于未按下状态（电平为0），则恢复“已释放”标志
    else if ((WKUP_STATUS == 0) && (KEY0_STATUS == 0) && (KEY_R_STATUS == 1) && (KEY_Y_STATUS == 1) && (KEY_G_STATUS == 1)) {
        key_release = 1;
    }

    return key_value;   // 返回检测到的按键值（无按键则返回 NONE_PRESS）
}
