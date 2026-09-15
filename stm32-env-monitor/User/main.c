#include  "./SYSTEM/sys/sys.h"
#include  "./SYSTEM/delay/delay.h"
#include  "./SYSTEM/usart/usart.h"
#include  "./BSP/LCD/lcd.h"
#include  "./TEXT/text.h"
#include  "./MALLOC/malloc.h"
#include  "./FATFS/exfuns/exfuns.h"
#include  "./BSP/OLED/oled.h"
#include  "./BSP/IIC/iic.h"
#include "./USMART/usmart.h"
#include "./BSP/OLED/test.h"
#include "./BSP/KEY/key-i.h"
#include "./BSP/SD/sdcard.h"
#include "./BSP/LED/led-i.h"


int  main (void){
    // 按键值（用于检测WKUP按键）
    uint8_t key;
    //函数返回值（用于字库更新）
    uint8_t res;
    //初始化HAL库
    HAL_Init();
    //初始化系统时钟(168MHz)
    sys_stm32_clk_init(336, 8, 2, 7);
    //初始化延时
    delay_init(168);
    //初始化串口
    usart_init(115200);
    //初始化LCD
    lcd_init(); 
    //初始化内部SRAM内存池（100KB，用于动态内存分配）
    my_mem_init(SRAM_IN);
    //初始化CCM内存池（60KB，仅CPU可访问）
    my_mem_init(SRAM_IN_CCM);
    //为exfuns申请内存（fs[0]和fs[1]文件系统对象）
    exfuns_init();
    //挂载SD卡到盘符 "0:"（1表示立即挂载）
    f_mount(fs[0], "0:", 1);
    //挂载NOR Flash到盘符 "1:"
    f_mount(fs[1], "1:", 1);
    //初始化字体
    fonts_init();
    //初始化USMART
    usmart_dev.init(84);
    //i2c初始化
    i2c_init();
    //oled初始化
    oled_init();
    //初始化按键（WKUP/KEY0/KEY1）
    key_i_init();
    //初始化LED（LED0对应PF9）
    led_i_init();
     
    OLED_Clear(0);
   
    OLED_ShowChinese(16, 20, "你好，Sb,我是你爸爸，听到没", 16, 1);
    lcd_text_show_string(30, 30, 200, 16, "正点原子STM32开发板", 16, 0, RED);
    lcd_text_show_string(30, 50, 200, 16, "GBK字库测试程序", 16, 0, RED);
    lcd_text_show_string(30, 70, 200, 16, "ALONE@WALKER", 16, 0, RED);
    lcd_text_show_string(30, 90, 200, 16, "~~！@#￥%1234你好", 16, 0, RED);

 while (fonts_init() != 0)           // 如果字库初始化失败，进入更新流程
        {
    UPD:                                    // ① 跳转标签（WKUP按键按下时跳转到这里）
        lcd_clear(WHITE);               // 清屏（白色背景）
        lcd_show_string(30, 30, 200, 16, 16, "STM32", RED);  // 显示标题

        /*
         * 【步骤1】初始化SD卡
         * SD卡中存放了字库文件（/SYSTEM/FONT/UNIGBK.BIN, GBK12.FON, GBK16.FON, GBK24.FON）
         * 必须成功初始化才能读取字库文件
         */
        while (sd_init() != 0)          // 如果SD卡初始化失败，循环提示
        {
            lcd_show_string(30, 30, 200, 16, 16, "SD Card Error!", RED);
            delay_ms(500);
            lcd_show_string(30, 30, 200, 16, 16, "Please Check! ", RED);
            delay_ms(500);
            LED0_TOGGLE();              // LED0翻转，指示错误状态
        }

        /* SD卡初始化成功 */
        lcd_show_string(30, 50, 200, 16, 16, "SD Card OK", RED);
        lcd_show_string(30, 70, 200, 16, 16, "Font Updating...", RED);

        /*
         * 【步骤2】从SD卡更新字库到NOR Flash
         * fonts_update_font(30, 90, 16, "0:", RED)
         *   参数1: X坐标起始位置
         *   参数2: Y坐标起始位置
         *   参数3: 进度提示文字大小
         *   参数4: 字库来源路径 "0:"（SD卡）
         *   参数5: 文字颜色
         *
         * 该函数会依次读取SD卡中的4个字库文件，写入NOR Flash
         * 执行时间较长（约10-30秒），期间LCD会显示进度百分比
         */
        res = fonts_update_font(30, 90, 16, (uint8_t *)"0:", RED);

        /* 如果字库更新失败，循环提示 */
        while (res != 0)
        {
            lcd_show_string(30, 90, 200, 16, 16, "Font Update Failed!", RED);
            delay_ms(200);
            lcd_show_string(30, 90, 200, 16, 16, "Please Check!      ", RED);
            delay_ms(200);
        }

        /* 字库更新成功 */
        lcd_show_string(30, 90, 200, 16, 16, "Font Update Success!   ", RED);
        delay_ms(1500);                 // 延时1.5秒，让用户看到成功信息
        lcd_clear(WHITE);               // 清屏，准备进入主界面

        /* 此时 while (fonts_init() != 0) 循环条件不满足，自动退出循环 */
    }


    while (1) {
            // 扫描按键（0表示不连续按下）
             key = key_i_scan(0); 
             if (key == WKUP_PRESS)  // 如果WKUP按键被按下（长按）
              {
               /* 跳转到字库更新标签，重新更新字库 */
               goto UPD;       // ② 跳转到字库更新流程（前面定义的标签）
             }
      
        }
}
