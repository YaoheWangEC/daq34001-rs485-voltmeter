#include "command_handler.h"
#include "command_table.h"
#include "bsp.h"
#include "global_vars.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static char resp[1024];

char* lscmd_handler(int argc, char **argv)
{
    // 帮助信息
    if (argc == 2 && 
       (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        sprintf(resp,
            "Usage: lscmd [options]\r\n"
            "Options:\r\n"
            "  -h, --help    Show this help message\r\n"
            "\r\n"
            "List all supported commands.\r\n");
        return resp;
    }
    
    if (argc == 1)
    {
        // 默认行为：列出所有命令
        int count = command_table_count();
        int offset = snprintf(resp, sizeof(resp), "Supported commands:\r\n");
        for (int i = 0; i < count && offset < sizeof(resp); i++)
        {
            const char* name = command_table_get_name(i);
            if (name)
            {
                offset += snprintf(resp + offset, sizeof(resp) - offset, "  %s\r\n", name);
            }
        }
        return resp;
    }

    // 无效参数
    sprintf(resp, "Invalid option. Try 'lscmd -h'\r\n");
    return resp;
}

char* echo_handler(int argc, char **argv)
{
    // 帮助信息
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        snprintf(resp, sizeof(resp),
                 "Usage: echo [text]\r\n"
                 "Options:\r\n"
                 "  -h, --help    Show this help message\r\n"
                 "\r\n"
                 "Print the given text to output.\r\n");
        return resp;
    }

    // 默认行为：拼接参数并返回
    if (argc > 1)
    {
        resp[0] = '\0';
        for (int i = 1; i < argc; i++)
        {
            strncat(resp, argv[i], sizeof(resp) - strlen(resp) - 1);
            if (i < argc - 1)
                strncat(resp, " ", sizeof(resp) - strlen(resp) - 1);
        }
        strncat(resp, "\r\n", sizeof(resp) - strlen(resp) - 1);
        return resp;
    }

    snprintf(resp, sizeof(resp), "\r\n");
    return resp;
}

char* volt_handler(int argc, char **argv)
{
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        sprintf(resp,
            "Usage: volt\r\n"
            "\r\n"
            "Read the latest voltage measurements from selected channel.\r\n");
        return resp;
    }

    if (g_vars.extadc_input_range == EXTADC_INPUT_HIGH_RANGE)
    {
        sprintf(resp, "%f\r\n", g_vars.input_high_range_v);
    }
    else
    {
        sprintf(resp, "%f\r\n", g_vars.input_low_range_v);
    }
    return resp;
}

char* range_handler(int argc, char **argv)
{
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        sprintf(resp,
            "Usage: range\r\n"
            "\r\n"
            "Query current input range.\r\n");
        return resp;
    }

    if (g_vars.extadc_input_range == EXTADC_INPUT_HIGH_RANGE)
        sprintf(resp, "high\r\n");
    else
        sprintf(resp, "low\r\n");
    return resp;
}

/** @brief 校准参数名 → 指针映射 */
typedef struct {
    const char *name;
    float *ptr;
} calib_param_t;

static const calib_param_t calib_params[] = {
    {"high_range_transfer.scale",     &g_vars.high_range_transfer.scale},
    {"high_range_transfer.offset",    &g_vars.high_range_transfer.offset},
    {"low_range_transfer.scale",      &g_vars.low_range_transfer.scale},
    {"low_range_transfer.offset",     &g_vars.low_range_transfer.offset},
    {"high_range_calibration.scale",  &g_vars.high_range_calibration.scale},
    {"high_range_calibration.offset", &g_vars.high_range_calibration.offset},
    {"low_range_calibration.scale",   &g_vars.low_range_calibration.scale},
    {"low_range_calibration.offset",  &g_vars.low_range_calibration.offset},
};

#define CALIB_PARAM_COUNT (sizeof(calib_params) / sizeof(calib_params[0]))

/**
 * @brief 校准参数管理
 *
 * calib              列出全部校准参数
 * calib set <n> <v>  设置指定参数
 * calib save         保存到 EEPROM
 * calib reset        恢复默认值
 */
char* calib_handler(int argc, char **argv)
{
    if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        sprintf(resp,
            "Usage: calib [set <name> <value> | save | reset]\r\n"
            "\r\n"
            "Manage calibration parameters.\r\n"
            "  calib              List all parameters\r\n"
            "  calib set <n> <v>  Set a parameter\r\n"
            "  calib save          Save to EEPROM\r\n"
            "  calib reset        Reset to defaults\r\n");
        return resp;
    }

    // 列出全部参数
    if (argc == 1)
    {
        int offset = 0;
        offset += snprintf(resp + offset, sizeof(resp) - offset,
            "high_range_transfer.scale=%f high_range_transfer.offset=%f\r\n",
            g_vars.high_range_transfer.scale, g_vars.high_range_transfer.offset);
        offset += snprintf(resp + offset, sizeof(resp) - offset,
            "low_range_transfer.scale=%f low_range_transfer.offset=%f\r\n",
            g_vars.low_range_transfer.scale, g_vars.low_range_transfer.offset);
        offset += snprintf(resp + offset, sizeof(resp) - offset,
            "high_range_calibration.scale=%f high_range_calibration.offset=%f\r\n",
            g_vars.high_range_calibration.scale, g_vars.high_range_calibration.offset);
        offset += snprintf(resp + offset, sizeof(resp) - offset,
            "low_range_calibration.scale=%f low_range_calibration.offset=%f\r\n",
            g_vars.low_range_calibration.scale, g_vars.low_range_calibration.offset);
        return resp;
    }

    // calib set <name> <value>
    if (argc == 4 && strcmp(argv[1], "set") == 0)
    {
        for (int i = 0; i < CALIB_PARAM_COUNT; i++)
        {
            if (strcmp(argv[2], calib_params[i].name) == 0)
            {
                *calib_params[i].ptr = (float)atof(argv[3]);
                sprintf(resp, "OK\r\n");
                return resp;
            }
        }
        sprintf(resp, "Unknown parameter: %s\r\n", argv[2]);
        return resp;
    }

    // calib save
    if (argc == 2 && strcmp(argv[1], "save") == 0)
    {
        if (global_vars_save())
            sprintf(resp, "Saved to EEPROM.\r\n");
        else
            sprintf(resp, "EEPROM write failed.\r\n");
        return resp;
    }

    // calib reset
    if (argc == 2 && strcmp(argv[1], "reset") == 0)
    {
        global_vars_reset();
        sprintf(resp, "Reset to defaults. Use 'calib save' to persist.\r\n");
        return resp;
    }

    sprintf(resp, "Invalid option. Try 'calib -h'\r\n");
    return resp;
}
