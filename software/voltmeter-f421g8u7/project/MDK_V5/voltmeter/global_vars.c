#include "global_vars.h"
#include "bsp.h"
#include <string.h>
#include <stdio.h>

#define GLOABL_VARS_IN_EEPROM_LENGTH (sizeof(global_vars_t) + 1)

global_vars_t g_vars;

/**
 * @brief 计算异或校验和，初始值0x55
 * 
 * @param buf 待计算的缓冲区
 * @param length_byte 缓冲区长度，以Byte计
 * @return uint8_t 校验值
 */
static uint8_t calc_xor_checksum(uint8_t *buf, uint32_t length_byte)
{
    uint8_t checksum = 0x55;

    for (uint32_t i = 0; i < length_byte; i++)
    {
        checksum = checksum ^ buf[i];
    }

    return checksum;
}

/**
 * @brief 写入默认值
 */
static void set_defaults(void)
{
    g_vars.high_range_transfer.scale = 101.0f / 3.0f;
    g_vars.high_range_transfer.offset = 0.0f;
    g_vars.low_range_transfer.scale = 2.0f / 3.0f;
    g_vars.low_range_transfer.offset = 0.0f;
    g_vars.high_range_calibration.scale = 1.0f;
    g_vars.high_range_calibration.offset = 0.0f;
    g_vars.low_range_calibration.scale = 1.0f;
    g_vars.low_range_calibration.offset = 0.0f;
    printf("load default\r\n");
}

/**
 * @brief 恢复全局变量为默认值 (仅 RAM, 不自动保存到 EEPROM)
 */
void global_vars_reset(void)
{
    set_defaults();
}

/**
 * @brief 初始化全局变量
 *
 * 尝试从 EEPROM 加载，失败则写入默认值。
 */
void global_vars_init(void)
{
    if (!global_vars_load())
    {
        printf("No preset global variables\r\n");
        set_defaults();
    }
    else
    {
        printf("Load preset global variables\r\n");
    }
}

bool global_vars_save(void)
{
    uint32_t save_length_byte = 0;
    uint8_t save_buf[GLOABL_VARS_IN_EEPROM_LENGTH];
    memset(save_buf, 0xff, sizeof(save_buf)); // 全部清空成0xff

    // 存入全局变量组
    save_length_byte += sizeof(g_vars);
    memcpy(save_buf, &g_vars, sizeof(g_vars));

    // 存入校验和
    save_length_byte += 1;
    save_buf[save_length_byte - 1] = calc_xor_checksum(save_buf, sizeof(g_vars));
    
    // 尝试存储
    if (eeprom_write_block(0x00, save_buf, save_length_byte))
    {
        printf("Successfully save all global variables into eeprom\r\n");
        return true;
    }
    else
    {
        printf("Can't save all global variables into eeprom\r\n");
        return false;
    }
}

bool global_vars_load(void)
{
    uint8_t load_buf[GLOABL_VARS_IN_EEPROM_LENGTH];
    global_vars_t *v = (global_vars_t *)load_buf;

    if (!eeprom_read_block(0x00, load_buf, GLOABL_VARS_IN_EEPROM_LENGTH))
    {
        printf("load failed\r\n");
        return false;
    }

    // 校验和
    uint8_t checksum = calc_xor_checksum(load_buf, sizeof(global_vars_t));
    if (checksum != load_buf[sizeof(global_vars_t)])
    {
        printf("load checksum error\r\n");
        return false;
    }

    // 数据合法性检查
    if (v->high_range_transfer.scale < 20.0f || v->high_range_transfer.scale > 200.0f) { printf("load range fail\r\n"); return false; }
    if (v->low_range_transfer.scale < 0.2f || v->low_range_transfer.scale > 5.0f) { printf("load range fail\r\n"); return false; }
    if (v->high_range_transfer.offset < -1.0f || v->high_range_transfer.offset > 1.0f) { printf("load range fail\r\n"); return false; }
    if (v->low_range_transfer.offset < -1.0f || v->low_range_transfer.offset > 1.0f) { printf("load range fail\r\n"); return false; }
    if (v->high_range_calibration.scale < 0.5f || v->high_range_calibration.scale > 2.0f) { printf("load range fail\r\n"); return false; }
    if (v->low_range_calibration.scale < 0.5f || v->low_range_calibration.scale > 2.0f) { printf("load range fail\r\n"); return false; }
    if (v->high_range_calibration.offset < -1.0f || v->high_range_calibration.offset > 1.0f) { printf("load range fail\r\n"); return false; }
    if (v->low_range_calibration.offset < -1.0f || v->low_range_calibration.offset > 1.0f) { printf("load range fail\r\n"); return false; }

    // 通过所有检查，恢复到 g_vars
    memcpy(&g_vars, v, sizeof(global_vars_t));
    printf("load success\r\n");
    return true;
}
