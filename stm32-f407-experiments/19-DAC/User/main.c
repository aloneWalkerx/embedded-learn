#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/KEY/key-i.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/ADC/adc.h"
#include "./BSP/DAC/dac.h"


extern DAC_HandleTypeDef g_dac_handle;
int main(void)
{ 
    uint8_t t = 0;
    uint8_t key;
    uint16_t dac_value;
    uint16_t dac_voltage = 100;
    uint16_t adc_value;
    uint16_t adc_voltage;
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 配置时钟，168MHz */
    delay_init(168);                    /* 初始化延时 */
    usart_init(115200);                 /* 初始化串口 */
    led_i_init();                       /* 初始化LED */
    key_i_init();                       /* 初始化按键 */
    lcd_init();                         /* 初始化LCD */
    adc_init();                         /* 初始化ADC */
    dac_init();                         /* 初始化DAC */
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "DAC TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    
    lcd_show_string(30, 110, 200, 16, 16, "WK_UP:+ KEY0:-", RED);
    
    lcd_show_string(30, 130, 200, 16, 16, "DAC_CH1 VAL:", BLUE);
    lcd_show_string(30, 150, 200, 16, 16, "DAC_CH1_VOL:0.000V", BLUE);
    lcd_show_string(30, 170, 200, 16, 16, "ADC2_CH1_VOL:0.000V", BLUE);
    
    dac_set_voltage(dac_voltage);       /* 设置DAC输出电压 */
    
    while (1)
    {
        key = key_i_scan(0);
        if (key == WKUP_PRESS)
        {
            /* 加大DAC输出 */
            if (dac_voltage < 3300)
            {
                dac_voltage += 100;
                dac_set_voltage(dac_voltage);
            }
        }
        else if (key == KEY0_PRESS)
        {
            /* 减小DAC输出 */
            if (dac_voltage > 100)
            {
                dac_voltage -= 100;
                dac_set_voltage(dac_voltage);
            }
        }
        
        if (++t == 20)
        {
            t = 0;
            
            /* 显示DAC输出数字量 */
            dac_value = HAL_DAC_GetValue(&g_dac_handle, DAC_DACX_CHY);
            lcd_show_xnum(126, 130, dac_value, 4, 16, 0, BLUE);
            
            /* 显示DAC输出模拟量 */
            lcd_show_xnum(126, 150, dac_voltage / 1000, 1, 16, 0, BLUE);
            lcd_show_xnum(142, 150, dac_voltage % 1000, 3, 16, 0x80, BLUE);
            
            /* 显示ADC采集模拟量 */
            adc_value = adc_get_result_average(ADC_ADCX_CHY, 10);
            adc_voltage = (adc_value * 3300) / 4095;
            lcd_show_xnum(134, 170, adc_voltage / 1000, 1, 16, 0, BLUE);
            lcd_show_xnum(150, 170, adc_voltage % 1000, 3, 16, 0x80, BLUE);
            
            LED0_TOGGLE();
        }
        
        delay_ms(10);
    }
}
