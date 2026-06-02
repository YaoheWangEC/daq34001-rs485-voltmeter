#ifndef __GLOBAL_VARS_H__
#define __GLOBAL_VARS_H__

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    EXTADC_INPUT_HIGH_RANGE = 0,
    EXTADC_INPUT_LOW_RANGE = 1
} extadc_input_range_t;

typedef struct 
{
    float scale; // 缩放系数
    float offset; // 偏置
} transfer_calibration_t;

/**
 * @brief 全局变量结构体
 */
typedef struct 
{
    extadc_input_range_t extadc_input_range;
    float input_high_range_v;
    float input_low_range_v;
    transfer_calibration_t high_range_transfer;
    transfer_calibration_t low_range_transfer;
    transfer_calibration_t high_range_calibration;
    transfer_calibration_t low_range_calibration;
} global_vars_t;

extern global_vars_t g_vars;

/**
 * @brief 初始化全局变量
 *
 * 尝试从 EEPROM 加载已保存的数据，若校验失败则使用默认值。
 */
void global_vars_init(void);

/**
 * @brief 将全局变量保存到 EEPROM
 * @return true 保存成功, false I2C 通信失败
 */
bool global_vars_save(void);

/**
 * @brief 从 EEPROM 加载全局变量
 * @return true 加载成功且校验通过, false 数据无效或通信失败
 */
bool global_vars_load(void);

/**
 * @brief 恢复全局变量为默认值 (仅 RAM, 不自动保存)
 */
void global_vars_reset(void);

#endif /* __GLOBAL_VARS_H__ */
