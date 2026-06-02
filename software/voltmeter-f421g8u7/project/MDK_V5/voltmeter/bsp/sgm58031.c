/**
 * @file    sgm58031.c
 * @brief   SGM58031 16位ΔΣ ADC 驱动实现
 * @details 通过 I2C 总线读写 SGM58031 内部寄存器, 提供初始化、通道/PGA/数据率配置、
 *          转换启停、原始值读取等功能。内部使用 i2c_application 库封装硬件操作。
 */

#include "sgm58031.h"
#include "at32f421_i2c.h"
#include "wk_i2c.h"
#include "i2c_application.h"

#include <string.h>

/**
 * @brief   从 SGM58031 读取单个寄存器
 * @param   hsgm58031  外设句柄指针
 * @param   reg_addr   寄存器地址 (0~6)
 * @param   data       输出寄存器值 (16位)
 * @return  true 成功, false I2C 通信失败或 data 为 NULL
 * @note    寄存器为 16 位, MSB first
 */
static bool sgm58031_read_register(sgm58031_handle_t *hsgm58031, uint8_t reg_addr, uint16_t *data)
{
    i2c_handle_type *hi2c = hsgm58031->hi2c;
    uint8_t dev_addr = hsgm58031->device_address;
    uint8_t buf[2];

    if (data == NULL) return false;

    i2c_status_type status = i2c_memory_read(hi2c,
                                             I2C_MEM_ADDR_WIDIH_8,
                                             dev_addr,
                                             reg_addr,
                                             buf,
                                             2,                  // 读取2字节
                                             1000);

    if (status == I2C_OK)
    {
        *data = ((uint16_t)buf[0] << 8) | buf[1];   // MSB first
        return true;
    }

    return false;
}

/**
 * @brief   向 SGM58031 写入单个寄存器
 * @param   hsgm58031  外设句柄指针
 * @param   reg_addr   寄存器地址 (0~6)
 * @param   data       待写入的 16 位值
 * @return  true 成功, false I2C 通信失败
 * @note    寄存器为 16 位, MSB first
 */
static bool sgm58031_write_register(sgm58031_handle_t *hsgm58031, uint8_t reg_addr, uint16_t data)
{
    i2c_handle_type *hi2c = hsgm58031->hi2c;
    uint8_t dev_addr = hsgm58031->device_address;
    uint8_t buf[2];

    buf[0] = (uint8_t)(data >> 8);       // MSB
    buf[1] = (uint8_t)(data & 0xFF);     // LSB

    i2c_status_type status = i2c_memory_write(hi2c,
                                              I2C_MEM_ADDR_WIDIH_8,
                                              dev_addr,
                                              reg_addr,
                                              buf,
                                              2,                  // 写入2字节
                                              1000);

    return (status == I2C_OK);
}

/**
 * @brief   SGM58031 初始化
 * @param   hsgm58031      外设句柄指针
 * @param   hi2c           绑定的 I2C 句柄
 * @param   device_address I2C 设备地址 (7位地址左移1位)
 * @return  true 成功, false I2C 通信失败
 * @note    清零句柄后绑定 I2C, 读取全部 7 个寄存器并缓存到 reg[] 数组,
 *          从 CONFIG 寄存器恢复 input/pga/datarate/continuous_mode 参数
 */
bool sgm58031_init(sgm58031_handle_t *hsgm58031,
                   i2c_handle_type *hi2c,
                   uint8_t device_address)
{
    memset(hsgm58031, 0x00, sizeof(*hsgm58031));

    hsgm58031->hi2c = hi2c;
    hsgm58031->device_address = device_address;

    bool ret = true;
    uint16_t data;
    memset(hsgm58031->reg, 0x00, sizeof(hsgm58031->reg));

    // 读出所有寄存器
    for (uint8_t reg_addr = 0; reg_addr < 7; reg_addr++)
    {
        ret &= sgm58031_read_register(hsgm58031, reg_addr, &data);
        if (ret == true)
        {
            hsgm58031->reg[reg_addr] = data;
        }
        else
        {
            return false;
        }
    }

    // 获取参数
    hsgm58031->input = (sgm58031_input_t)(hsgm58031->reg[SGM58031_REG_CONFIG] & SGM58031_MUX_MASK);
    hsgm58031->pga = (sgm58031_pga_t)(hsgm58031->reg[SGM58031_REG_CONFIG] & SGM58031_PGA_MASK);
    hsgm58031->datarate = (sgm58031_datarate_t)(hsgm58031->reg[SGM58031_REG_CONFIG] & SGM58031_DR_MASK);
    hsgm58031->continuous_mode = !(hsgm58031->reg[SGM58031_REG_CONFIG] & SGM58031_MODE_MASK);

    return true;
}

/**
 * @brief   SGM58031 配置参数
 * @param   hsgm58031     外设句柄指针
 * @param   input_channel 输入通道
 * @param   pga           PGA 增益 (满量程)
 * @param   datarate      数据率 (SPS)
 * @return  true 成功, false I2C 通信失败
 * @note    读取-修改-写入 CONFIG 寄存器, 仅修改 MUX/PGA/DR 相关位,
 *          同时强制 COMP_QUE = 11 禁用比较器并将 ALERT/RDY 引脚配置为
 *          数据就绪指示 (每次转换完成输出 8us 低脉冲)
 */
bool sgm58031_config(sgm58031_handle_t *hsgm58031,
                     sgm58031_input_t input_channel,
                     sgm58031_pga_t pga,
                     sgm58031_datarate_t datarate)
{
    bool ret = true;
    uint16_t data;

    ret &= sgm58031_read_register(hsgm58031, SGM58031_REG_CONFIG, &data);
    data &= ~(SGM58031_MUX_MASK | SGM58031_PGA_MASK | SGM58031_DR_MASK); // 清空对应位
    data |= (input_channel | pga | datarate); // 写入对应位
    data |= 0x0003; // COMP_QUE = 11 禁用比较器功能，ALERT/RDY引脚只在每次转换完成后输出8us低电平
    ret &= sgm58031_write_register(hsgm58031, SGM58031_REG_CONFIG, data);

    if (ret == true)
    {
        hsgm58031->reg[SGM58031_REG_CONFIG] = data;
        hsgm58031->input = input_channel;
        hsgm58031->pga = pga;
        hsgm58031->datarate = datarate;
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief   SGM58031 启动/停止转换
 * @param   hsgm58031  外设句柄指针
 * @param   enable     true=连续转换模式, false=单次转换模式
 * @return  true 成功, false I2C 通信失败
 * @note    仅当请求的模式与当前模式不同时才操作寄存器, 避免冗余 I2C 写入
 */
bool sgm58031_start(sgm58031_handle_t *hsgm58031, bool enable)
{
    bool ret = true;
    uint16_t data;

    if ((enable == true) && (hsgm58031->continuous_mode == false))
    {
        ret &= sgm58031_read_register(hsgm58031, SGM58031_REG_CONFIG, &data);
        data &= ~SGM58031_MODE_MASK; // MODE位置0启动持续转换模式
        ret &= sgm58031_write_register(hsgm58031, SGM58031_REG_CONFIG, data);
    }
    else if ((enable == false) && (hsgm58031->continuous_mode == true))
    {
        ret &= sgm58031_read_register(hsgm58031, SGM58031_REG_CONFIG, &data);
        data |= SGM58031_MODE_MASK; // MODE位置1启动单次转换模式
        ret &= sgm58031_write_register(hsgm58031, SGM58031_REG_CONFIG, data);
    }

    if (ret == true)
    {
        hsgm58031->reg[SGM58031_REG_CONFIG] = data;
        hsgm58031->continuous_mode = enable;
    }

    return ret;
}

/**
 * @brief   读取 SGM58031 当前转换原始值
 * @param   hsgm58031  外设句柄指针
 * @param   value      输出原始 ADC 值 (int16_t, 二进制补码, 符号位即 MSB)
 * @return  true 成功, false I2C 通信失败
 * @note    读取 CONVERSION 寄存器并更新 reg[] 缓存。连续模式下直接返回最新
 *          转换结果, 单次模式下需在外部等待转换完成
 */
bool sgm58031_read_raw(sgm58031_handle_t *hsgm58031, int16_t *value)
{
    bool ret = true;
    uint16_t data;

    ret = sgm58031_read_register(hsgm58031, SGM58031_REG_CONVERSION, &data);

    if (ret == true)
    {
        hsgm58031->reg[SGM58031_REG_CONVERSION] = data;
        *value = data;
    }

    return ret;
}
