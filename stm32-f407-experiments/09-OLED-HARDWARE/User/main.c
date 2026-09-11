#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/I2C/iic.h"
#include "./BSP/OLED/oled.h"
#include "./BSP/OLED/test.h"

int main(void)
{
    HAL_Init();
    sys_stm32_clock_init(336, 8, 2, 7);
    delay_init(168);
    usart_init(115200);
    led_i_init();

    /* Ó²¼þ I2C ³õÊ¼»¯ */
    MX_I2C1_Init();

    OLED_Init();
    OLED_Clear(0);

    while (1) {
        TEST_MainPage();
        OLED_Clear(0);
        delay_ms(500);

        Test_Color();
        OLED_Clear(0);
        delay_ms(500);

        Test_Rectangular();
        OLED_Clear(0);
        delay_ms(500);

        Test_Circle();
        OLED_Clear(0);
        delay_ms(500);

        Test_Triangle();
        OLED_Clear(0);
        delay_ms(500);

        TEST_English();
        OLED_Clear(0);
        delay_ms(500);

        TEST_Number_Character();
        OLED_Clear(0);
        delay_ms(500);

        TEST_Chinese();
        OLED_Clear(0);
        delay_ms(500);

        TEST_BMP();
        OLED_Clear(0);
        delay_ms(500);

        TEST_Menu1();
        OLED_Clear(0);
        delay_ms(500);

        TEST_Menu2();
        OLED_Clear(0);
        delay_ms(500);
    }
}
