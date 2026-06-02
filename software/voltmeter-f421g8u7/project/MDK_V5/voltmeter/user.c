#include "user.h"
#include "wk_system.h"
#include "global_vars.h"
#include "bsp.h"
#include "lcd.h"
#include "colorlut.h"
#include "command_io.h"
#include "command_parser.h"

#include <stdio.h>
#include <string.h>

#define PLT_BACKGROUND 0
#define PLT_CH1 1
#define PLT_CH2 2
#define PLT_FOREGROUND 3

static uint8_t lcd_frame[LCD_H * LCD_W / 2];

static void voltmeter_get_data_handler(void);
static void voltmeter_display_handler(void);

void setup(void)
{
    printf("setup\r\n");
	bsp_init();
	global_vars_init();
    command_io_init();
    command_parser_init();

    // 初始化 LCD
    uint16_t palette[] = {COLOR_WHITE, 0xfbe0, 0x001f, COLOR_BLACK};
	lcd_init();
	lcd_fill(0, 0, 160, 80, COLOR_WHITE);
    lcd_show_string(10, 30, (const uint8_t *)"Hello World!", COLOR_BLUE, COLOR_WHITE, FONTSIZE_2412, 1);
    wk_delay_ms(500);

    colorlut_init(palette, sizeof(palette) / sizeof(uint16_t));
    colorlut_clear_buffer(lcd_frame, LCD_W, LCD_H, 0);
    colorlut_flush_to_lcd(lcd_frame, 0, 0, 160, 80);
}

void loop(void)
{
    voltmeter_get_data_handler();
    voltmeter_display_handler();

    command_parser_task();
}

static void voltmeter_get_data_handler(void)
{
    // 读取 ADC 侧电压
    float v_adc = extadc_read_voltage();

    // 获取当前量程的传递函数和校准参数
    transfer_calibration_t *tf;
    transfer_calibration_t *cal;
    if (g_vars.extadc_input_range == EXTADC_INPUT_HIGH_RANGE)
    {
        tf = &g_vars.high_range_transfer;
        cal = &g_vars.high_range_calibration;
    }
    else
    {
        tf = &g_vars.low_range_transfer;
        cal = &g_vars.low_range_calibration;
    }

    // 换算为采集侧电压: 先传递函数, 再校准
    float v_input = (v_adc * tf->scale + tf->offset) * cal->scale + cal->offset;

    // 存入全局变量
    if (g_vars.extadc_input_range == EXTADC_INPUT_HIGH_RANGE)
    {
        g_vars.input_high_range_v = v_input;
    }
    else
    {
        g_vars.input_low_range_v = v_input;
    }

    // 挡位选择
    bool key_sel = key_read_sel();

    if (((key_sel == true) && (g_vars.extadc_input_range == EXTADC_INPUT_HIGH_RANGE)) ||
        ((key_sel == false) && (g_vars.extadc_input_range == EXTADC_INPUT_LOW_RANGE)))
    {
        extadc_set_channel(key_sel);
    }
}

static void voltmeter_display_handler(void)
{
    char buf[32];

    // 清缓冲
    colorlut_clear_buffer(lcd_frame, LCD_W, LCD_H, PLT_BACKGROUND);

    // 通道标签
    bool is_high = (g_vars.extadc_input_range == EXTADC_INPUT_HIGH_RANGE);
    uint8_t plt_ch = is_high ? PLT_CH1 : PLT_CH2;
    const char *ch_text = is_high ? "CH1" : "CH2";

    colorlut_fill_rect(lcd_frame, 0, 0, 22, 14, plt_ch);
    colorlut_show_string(lcd_frame, 2, 1, (const uint8_t *)ch_text, PLT_FOREGROUND, FONTSIZE_1206);

    // 电压行
    const char *h_prefix = is_high ? ">" : " ";
    const char *l_prefix = is_high ? " " : ">";

    // 高量程: V
    sprintf(buf, "%sH %+08.3f V", h_prefix, g_vars.input_high_range_v);
    colorlut_show_string(lcd_frame, 0, 16, (const uint8_t *)buf, PLT_CH1, FONTSIZE_2412);

    // 低量程: mV
    sprintf(buf, "%sL %+08.2fmV", l_prefix, g_vars.input_low_range_v * 1000.0f);
    colorlut_show_string(lcd_frame, 0, 40, (const uint8_t *)buf, PLT_CH2, FONTSIZE_2412);

    // 刷新到 LCD
    colorlut_flush_to_lcd(lcd_frame, 0, 0, 160, 80);
}

