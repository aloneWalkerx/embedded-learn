#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/LED/led-i.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/ADC/adc.h"

#define ADC_NCH_DMA_BUF_SIZE    (50 * ADC_NCH_DMA_ADCX_CH_NUM)
uint16_t g_adc_nch_dma_buf[ADC_NCH_DMA_BUF_SIZE];
extern uint8_t g_adc_nch_dma_sta;

int main(void)
{
    uint16_t adc_result[ADC_NCH_DMA_ADCX_CH_NUM];
    uint16_t voltage;
    uint16_t index;
    uint32_t result_sum;
    uint8_t ch_index;
    
    HAL_Init();                                     /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7);             /* 配置时钟，168MHz */
    delay_init(168);                                /* 初始化延时 */
    usart_init(115200);                             /* 初始化串口 */
    led_i_init();                                     /* 初始化LED */
    lcd_init();                                     /* 初始化LCD */
    adc_nch_dma_init((uint32_t)g_adc_nch_dma_buf);  /* 初始化多通道ADC DMA读取 */
    
    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "ADC NCH DMA TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    
    lcd_show_string(30, 110, 200, 16, 16, "ADC1_CH5_VAL:0", BLUE);
    lcd_show_string(30, 130, 200, 16, 16, "ADC1_CH5_VOL:0.000V", BLUE);
    lcd_show_string(30, 150, 200, 16, 16, "ADC1_CH6_VAL:0", BLUE);
    lcd_show_string(30, 170, 200, 16, 16, "ADC1_CH6_VOL:0.000V", BLUE);
    
    /* 开启多通道ADC DMA读取 */
    adc_nch_dma_enable(ADC_NCH_DMA_BUF_SIZE);
    
    while (1)
    {
        if (g_adc_nch_dma_sta == 1)
        {
            g_adc_nch_dma_sta = 0;
            
            for (ch_index=0; ch_index<ADC_NCH_DMA_ADCX_CH_NUM; ch_index++)
            {
                /* 对DMA读取的多个ADC数据进行均值滤波 */
                for (result_sum = 0, index=0; index<(ADC_NCH_DMA_BUF_SIZE / ADC_NCH_DMA_ADCX_CH_NUM); index++)
                {
                    result_sum += g_adc_nch_dma_buf[(ADC_NCH_DMA_ADCX_CH_NUM * index) + ch_index];
                }
                adc_result[ch_index] = result_sum / (ADC_NCH_DMA_BUF_SIZE / ADC_NCH_DMA_ADCX_CH_NUM);
            }
            
            /* 计算实际电压值（扩大1000倍） */
            voltage = (adc_result[0] * 3300) / 4095;
            lcd_show_xnum(134, 110, adc_result[0], 5, 16, 0, BLUE);
            lcd_show_xnum(134, 130, voltage / 1000, 1, 16, 0, BLUE);
            lcd_show_xnum(150, 130, voltage % 1000, 3, 16, 0x80, BLUE);
            
            /* 计算实际电压值（扩大1000倍） */
            lcd_show_xnum(134, 150, adc_result[1], 5, 16, 0, BLUE);
            voltage = (adc_result[1] * 3300) / 4095;
            lcd_show_xnum(134, 170, voltage / 1000, 1, 16, 0, BLUE);
            lcd_show_xnum(150, 170, voltage % 1000, 3, 16, 0x80, BLUE);
            
            adc_nch_dma_enable(ADC_NCH_DMA_BUF_SIZE);
        }
        
        LED0_TOGGLE();
        
        delay_ms(100);
    }
}


