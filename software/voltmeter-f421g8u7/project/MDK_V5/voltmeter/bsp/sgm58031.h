/**
 * @file    sgm58031.h
 * @brief   SGM58031 16位低功耗ΔΣ ADC 驱动
 * @details 通过 I2C 接口驱动 SGM58031，支持差分输入、PGA配置、数据率选择、
 *          连续/单次转换模式。
 *
 * @note   I2C 地址: 7位地址 0x48（ADDR接GND）
 *         0x49（ADDR接Vdd） 0x4a（ADDR接SDA）
 *         0x4b（ADDR接SCL）,传入时左移一位
 *         寄存器位宽: 16位 MSB first
 */

#ifndef __SGM58031_H__
#define __SGM58031_H__

#include "i2c_application.h"
#include <stdint.h>
#include <stdbool.h>

/** @name 寄存器地址
  * @{
  */

#define SGM58031_REG_CONVERSION    0x00  /*!< 转换结果寄存器 (只读) */
#define SGM58031_REG_CONFIG        0x01  /*!< 配置寄存器 (读写) */
#define SGM58031_REG_LO_THRESH     0x02  /*!< 低阈值寄存器 (读写) */
#define SGM58031_REG_HI_THRESH     0x03  /*!< 高阈值寄存器 (读写) */
#define SGM58031_REG_CONFIG1       0x04  /*!< 配置寄存器1 (读写) */
#define SGM58031_REG_CHIP_ID       0x05  /*!< 芯片ID寄存器 (只读) */
#define SGM58031_REG_GN_TRIM1      0x06  /*!< 增益微调寄存器1 (读写) */

/**
  * @}
  */

/** @name CONFIG 寄存器位掩码
  * @{
  */

#define SGM58031_OS_MASK           (1u << 15)  /*!< 操作状态/启动转换位 */
#define SGM58031_MUX_MASK          (7u << 12)  /*!< 输入通道选择 [14:12] */
#define SGM58031_PGA_MASK          (7u << 9)   /*!< 可编程增益放大器 [11:9] */
#define SGM58031_MODE_MASK         (1u << 8)   /*!< 工作模式 [8] */
#define SGM58031_DR_MASK           (7u << 5)   /*!< 数据率 [7:5] */

/**
  * @}
  */

/** 
 * @brief 输入通道选择
 */
typedef enum
{
    SGM58031_INPUT_AIN0_AIN1 = (0u << 12),  /*!< 差分 AIN0(+) - AIN1(-) (默认) */
    SGM58031_INPUT_AIN2_AIN3 = (3u << 12)   /*!< 差分 AIN2(+) - AIN3(-) */
} sgm58031_input_t;

/** 
 * @brief 可编程增益放大器 (PGA) 满量程范围 
 */
typedef enum
{
    SGM58031_PGA_6_144V = (0u << 9),  /*!< FS = ±6.144V */
    SGM58031_PGA_4_096V = (1u << 9),  /*!< FS = ±4.096V (默认) */
    SGM58031_PGA_2_048V = (2u << 9),  /*!< FS = ±2.048V */
    SGM58031_PGA_1_024V = (3u << 9),  /*!< FS = ±1.024V */
    SGM58031_PGA_0_512V = (4u << 9),  /*!< FS = ±0.512V */
    SGM58031_PGA_0_256V = (5u << 9)   /*!< FS = ±0.256V */
}sgm58031_pga_t;

/** 
 * @brief 数据率 (SPS = Samples Per Second)
 */
typedef enum
{
    SGM58031_DR_6_25_SPS  = (0u << 5),  /*!<   6.25 SPS */
    SGM58031_DR_12_5_SPS  = (1u << 5),  /*!<  12.5  SPS */
    SGM58031_DR_25_SPS    = (2u << 5),  /*!<  25    SPS */
    SGM58031_DR_50_SPS    = (3u << 5),  /*!<  50    SPS */
    SGM58031_DR_100_SPS   = (4u << 5),  /*!< 100    SPS (默认) */
    SGM58031_DR_200_SPS   = (5u << 5),  /*!< 200    SPS */
    SGM58031_DR_400_SPS   = (6u << 5),  /*!< 400    SPS */
    SGM58031_DR_800_SPS   = (7u << 5)   /*!< 800    SPS */
} sgm58031_datarate_t;

/** 
 * @brief SGM58031 外设句柄
 */
typedef struct
{
    i2c_handle_type *hi2c;              /*!< 绑定的 I2C 句柄 */
    uint8_t device_address;             /*!< I2C 设备地址 (7位地址左移1位) */
    uint16_t reg[7];                    /*!< 寄存器镜像缓存 */
    sgm58031_input_t input;             /*!< 当前输入通道配置 */
    sgm58031_pga_t pga;                 /*!< 当前 PGA 配置 */
    sgm58031_datarate_t datarate;       /*!< 当前数据率配置 */
    bool continuous_mode;               /*!< 连续转换模式使能标志 */
} sgm58031_handle_t;

/**
 * @brief   SGM58031 初始化
 * @param   hsgm58031      外设句柄指针
 * @param   hi2c           绑定的 I2C 句柄
 * @param   device_address I2C 设备地址
 * @return  true 初始化成功, false I2C 通信失败
 * @note    该函数会读取全部 7 个寄存器到缓存, 并从 CONFIG 寄存器恢复
 *          当前的 input/pga/datarate/continuous_mode 参数
 */
bool sgm58031_init(sgm58031_handle_t *hsgm58031,
                   i2c_handle_type *hi2c,
                   uint8_t device_address);

/**
 * @brief   SGM58031 配置参数
 * @param   hsgm58031      外设句柄指针
 * @param   input_channel  输入通道
 * @param   pga            PGA 增益
 * @param   datarate       数据率
 * @return  true 配置成功, false I2C 通信失败
 * @note    自动将 COMP_QUE 置 11, 禁用比较器并将 ALERT/RDY 引脚配置为
 *          数据就绪指示 (每次转换完成后输出 8us 低脉冲)
 */
bool sgm58031_config(sgm58031_handle_t *hsgm58031,
                     sgm58031_input_t input_channel,
                     sgm58031_pga_t pga,
                     sgm58031_datarate_t datarate);

/**
 * @brief   SGM58031 启动/停止转换
 * @param   hsgm58031  外设句柄指针
 * @param   enable     true=连续转换模式, false=单次转换模式
 * @return  true 操作成功, false I2C 通信失败
 * @note    只有模式实际发生切换时才发起 I2C 写入
 */
bool sgm58031_start(sgm58031_handle_t *hsgm58031, bool enable);

/**
 * @brief   读取 SGM58031 原始转换值
 * @param   hsgm58031  外设句柄指针
 * @param   value      输出原始 ADC 值 (int16_t, 二进制补码)
 * @return  true 读取成功, false I2C 通信失败
 */
bool sgm58031_read_raw(sgm58031_handle_t *hsgm58031, int16_t *value);

#endif /* __SGM58031_H__ */
