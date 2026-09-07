#ifndef  KEY_I_E_H
#define  KEY_I_E_H

#include "./SYSTEM/sys/sys.h"

//3，键值定义
//没有任何按键按下
#define     I_E_NONE_PRESS                  0


//按下WKUP
#define     I_E_WKUP_PRESS                  1

//按下KEY0
#define     I_E_KEY0_PRESS                  2
//按下KEYR
#define     I_E_KEY_R_PRESS                 3

//按下KEYY
#define     I_E_KEY_Y_PRESS                 4

//按下KEYG
#define     I_E_KEY_G_PRESS                 5


//扫描按键
uint8_t key_i_e_scan(uint8_t mode);

#endif
