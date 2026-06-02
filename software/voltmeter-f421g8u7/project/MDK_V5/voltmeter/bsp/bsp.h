#ifndef __BSP_H__
#define __BSP_H__

#include "i2c_application.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief BSP 初始化入口
 *
 * 初始化 I2C2 外设句柄并调用 i2c_config 完成底层配置。
 * 在系统启动时调用，用于准备板级支持包。
 */
void bsp_init(void);

/**
 * @brief I2C 接口底层初始化
 *
 * 根据传入的 I2C 句柄选择具体的初始化函数。
 *
 * @param hi2c I2C 外设句柄
 */
void i2c_lowlevel_init(i2c_handle_type* hi2c);

/**
 * @brief 向 EEPROM 批量写入数据
 *
 * @param addr 起始地址
 * @param buf  指向待写入数据的缓冲区
 * @param len  待写入的字节数
 * @return true=写入成功, false=写入失败
 */
bool eeprom_write_block(uint16_t addr, const uint8_t *buf, uint16_t len);

/**
 * @brief 从 EEPROM 批量读取数据
 *
 * @param addr 起始地址
 * @param buf  指向存放读取结果的缓冲区
 * @param len  待读取的字节数
 * @return true=读取成功, false=读取失败
 */
bool eeprom_read_block(uint16_t addr, uint8_t *buf, uint16_t len);

/**
 * @brief 读取 SGM58031 当前转换电压值
 * @return 电压值，单位：V（如 1.234 表示 1.234V）
 * @note  调用前需确保已通过 bsp_init() 初始化并启动了连续转换
 */
float extadc_read_voltage(void);

/**
 * @brief  读取量程选择引脚电平
 * @return true=SEL 高电平, false=SEL 低电平
 * @note   SEL 引脚在 wk_gpio_config 中已配置为输入上拉
 */
bool key_read_sel(void);

/**
 * @brief  根据 SEL 引脚电平切换 ADC 输入通道
 * @param  sel_high true=差分通道2 (AIN2-AIN3), false=差分通道1 (AIN0-AIN1)
 * @return true 切换成功, false I2C 通信失败
 * @note   仅修改输入通道, 保持当前 PGA 和数据率不变
 */
bool extadc_set_channel(bool sel_high);

#endif /* __BSP_H__ */
