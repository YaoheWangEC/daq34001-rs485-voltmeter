#include "bsp.h"
#include "global_vars.h"

#include "at32f421_i2c.h"
#include "wk_i2c.h"
#include "i2c_application.h"
#include "eeprom.h"

#include "sgm58031.h"

#include "at32f421_wk_config.h"
#include "at32f421_gpio.h"
// #include "at32f421_dma.h"
// #include "wk_dma.h"
// #include "at32f421_spi.h"
// #include "wk_system.h"

#include <stdio.h>
#include <string.h>

#define EEPROM_ADDRESS 0xA0
#define SGM58031_ADDRESS 0x90

i2c_handle_type hi2c1_adc;
i2c_handle_type hi2c2_eeprom;

sgm58031_handle_t hsgm58031;

/**
 * @brief BSP 初始化
 *
 * 初始化 I2C、EEPROM 和 SGM58031。
 * 根据 SEL 引脚电平选择 ADC 输入通道:
 *   - SEL 低电平 → 差分通道1 (AIN0-AIN1)
 *   - SEL 高电平 → 差分通道2 (AIN2-AIN3)
 */
void bsp_init(void)
{
    hi2c1_adc.i2cx = I2C1;
    hi2c2_eeprom.i2cx = I2C2;
    i2c_config(&hi2c1_adc);
    i2c_config(&hi2c2_eeprom);

    sgm58031_init(&hsgm58031, &hi2c1_adc, SGM58031_ADDRESS);
    sgm58031_config(&hsgm58031, SGM58031_INPUT_AIN0_AIN1, SGM58031_PGA_2_048V, SGM58031_DR_6_25_SPS);
    extadc_set_channel(key_read_sel());
    sgm58031_start(&hsgm58031, true);
}

/**
 * @brief I2C 接口底层初始化
 *
 * 根据传入的 I2C 句柄选择具体的初始化函数。
 *
 * @param hi2c I2C 外设句柄
 */
void i2c_lowlevel_init(i2c_handle_type* hi2c)
{
    if (hi2c->i2cx == I2C1)
    {
        wk_i2c1_init();
    }
    else if (hi2c->i2cx == I2C2)
    {
        wk_i2c2_init();
    }
}

/**
 * @brief 向 EEPROM 批量写入数据
 *
 * @param addr 起始地址
 * @param buf  指向待写入数据的缓冲区
 * @param len  待写入的字节数
 * @return true=写入成功, false=写入失败
 */
bool eeprom_write_block(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    if (buf == NULL || (addr + len) > 256) return false;

    i2c_status_type status = eeprom_write_buffer(&hi2c2_eeprom,
                                                 EE_MODE_POLL,
                                                 I2C_MEM_ADDR_WIDIH_8,
                                                 EEPROM_ADDRESS,
                                                 addr,
                                                 (uint8_t*)buf,
                                                 len,
                                                 1000);
    return (status == I2C_OK);
}

/**
 * @brief 从 EEPROM 批量读取数据
 *
 * @param addr 起始地址
 * @param buf  指向存放读取结果的缓冲区
 * @param len  待读取的字节数
 * @return true=读取成功, false=读取失败
 */
bool eeprom_read_block(uint16_t addr, uint8_t *buf, uint16_t len)
{
    if (buf == NULL || (addr + len) > 256) return false;

    i2c_status_type status = eeprom_read_buffer(&hi2c2_eeprom,
                                                EE_MODE_POLL,
                                                I2C_MEM_ADDR_WIDIH_8,
                                                EEPROM_ADDRESS,
                                                addr,
                                                buf,
                                                len,
                                                1000);
    return (status == I2C_OK);
}

/**
 * @brief 读取 SGM58031 当前转换电压值
 *
 * 从连续转换中读取最近的 ADC 结果，根据当前 PGA 配置换算为电压。
 * PGA 枚举值右移 9 位得到索引 0~5，对应不同的满量程电压。
 *
 * @return float 电压值 (单位: V)，失败时返回 0.0f
 */
float extadc_read_voltage(void)
{
    int16_t raw = 0;

    if (!sgm58031_read_raw(&hsgm58031, &raw))
    {
        return 0.0f;
    }

    // 获取 PGA 索引
    uint8_t pga_index = (uint8_t)(hsgm58031.pga >> 9);

    // 查表得到满量程电压
    float fs;
    switch (pga_index)
    {
        case 0:  fs = 6.144f; break;
        case 1:  fs = 4.096f; break;
        case 2:  fs = 2.048f; break;
        case 3:  fs = 1.024f; break;
        case 4:  fs = 0.512f; break;
        case 5:  fs = 0.256f; break;
        default: fs = 2.048f; break;
    }

    return (float)raw * fs / 32768.0f;
}

/**
 * @brief  读取量程选择引脚电平
 * @return true=SEL 高电平, false=SEL 低电平
 * @note   SEL 引脚 (PB4) 在 wk_gpio_config 中已配置为输入上拉
 */
bool key_read_sel(void)
{
    return (gpio_input_data_bit_read(SEL_GPIO_PORT, SEL_PIN) == SET);
}

/**
 * @brief  根据 SEL 电平切换 ADC 输入通道
 * @param  sel_high true=差分通道1 (AIN0-AIN1), false=差分通道2 (AIN2-AIN3)
 * @return true 切换成功, false I2C 通信失败
 * @note   仅修改输入通道, 保持当前 PGA 和数据率不变
 */
bool extadc_set_channel(bool sel_high)
{
    sgm58031_input_t input = sel_high ? SGM58031_INPUT_AIN0_AIN1 : SGM58031_INPUT_AIN2_AIN3;
 
    bool ret = sgm58031_config(&hsgm58031, input, hsgm58031.pga, hsgm58031.datarate);

    if (ret == true)
    {
        g_vars.extadc_input_range = sel_high ? EXTADC_INPUT_LOW_RANGE : EXTADC_INPUT_HIGH_RANGE;
    }
    else
    {
        // do nothing
    }
    return ret;
}
