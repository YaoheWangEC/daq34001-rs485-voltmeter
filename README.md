# DAQ34001 — RS485 双量程电压采集卡

> RS485 Dual-Range Voltage Acquisition Card

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

## 简介 | Introduction

DAQ34001 是一款基于 **AT32F421** 的双量程 RS485 电压采集卡，内置 16-bit ADC，支持高/低两个电压量程手动切换，通过 RS485 总线以类 CLI 命令进行控制和数据读取。

DAQ34001 is a dual-range RS485 voltage acquisition card based on the **AT32F421** MCU. It features a 16-bit ADC with manual high/low range switching and communicates via RS485 using CLI-like commands. 

![DAQ34001](resource/overview.jpg)

> 视频演示：*待补充* ｜ [立创开源](https://oshwhub.com/waterbird/project_tgelpqyb)

## 主要参数 | Specifications

| 参数 | 值 | 备注 |
| ------ | ------------- | ----------------------- |
| 主控 MCU | AT32F421G8U7  | ARM Cortex-M4F |
| ADC | SGM58031      | 16-bit ΔΣ, I2C 接口       |
| 高量程 | ±60 V         | 分压网络前端                  |
| 低量程    | ±1 V          | 直通前端                    |
| 通信接口   | RS485 CLI     | 115200 / 8N1            |
| 显示     | 160×80 彩色 LCD | SPI 接口                  |
| 供电     | 5V DC         | 非隔离                     |
| 线性度   | 3.5digit | 实验测量值，不保证长期可靠 |

---

## 快速开始 | Quick Start

### 1. 接线

| 端子        | 功能       | 备注                   |
| --------- | -------- | -------------------- |
| VIN / GND | 电源输入 | 5V |
| A / B     | RS485 总线 | 可选焊接120Ω终端电阻 |
| CH+ / CH- | 信号输入 | 差分输入 |

### 2. 上电

接通电源后，LCD 显示 `Hello World!`，随后进入测量界面。

### 3. 测量连接

将待测电压接入测量端子，注意两个量程的COM端内部短接在一起，切勿让其接入不同电位。拨动开关选择正确的量程。

### 4. RS485 连接

使用 USB 转 RS485 模块连接电脑：

```
PC (USB) -> USB-RS485 模块 -> DAQ34001
```

串口参数：**115200 bps, 8 data bits, 1 stop bit, no parity**

### 5. 发送命令

打开串口终端（推荐 [SuperCom](https://github.com/SuperStudio/SuperCom) 或其他），发送：

```bash
lscmd
```

返回：

```
Supported commands:
  lscmd
  echo
  volt
  range
  calib
```

尝试读取当前电压：

```bash
volt
```

## RS485 命令协议 | Command Protocol

DAQ34001 采用 Linux CLI 风格的 ASCII 文本协议。

### 命令列表 | Command List

| 命令    | 参数                     | 说明                      |
| ------- | ------------------------ | ----------------------- |
| `lscmd` | `-h`                     | 列出所有命令                  |
| `echo`  | `<text>`                 | 回显参数                    |
| `volt`  | —                        | 读取当前量程电压值               |
| `range` | —                        | 查询当前量程 (`high` / `low`) |
| `calib` | `set` / `save` / `reset` | 校准参数管理                  |

### 帧格式 | Frame Format

- 命令以 `\r\n` 结束
- 最大长度：64 字节
- 响应以 `\r\n` 结束
- 半双工

## 目录结构 | Directory Structure

```
daq34001-rs485-voltmeter/
├── README.md                    # 项目说明 (本文件)
├── LICENSE                      # MIT 许可证
├── hardware/                    # 硬件设计
│   ├── *.epro2                   #   立创EDA 工程源文件
│   ├── SCH_*.pdf                 #   原理图 (PDF)
│   ├── PCB_*.pdf                 #   PCB 布局 (PDF)
│   └── Gerber_*.zip             #   生产文件 (Gerber)
├── mechanical/                  # 结构设计
│   ├── *.STEP                   #   STEP 通用格式
│   └── *.DWG                    #   2D 工程图
├── software/                    # 固件源码
│   └── voltmeter-f421g8u7/      #   Keil MDK 工程
│       ├── libraries/           #     CMSIS + AT32 外设库
│       ├── middlewares/         #     FreeRTOS, FatFS, I2C
│       └── project/             #     应用代码
└── resource/                    # 资源 (照片、截图等)
```

---

## 许可证 | License

本项目由不同许可的组件构成：

- **应用代码**（`project/src/voltmeter/` 下用户编写的业务逻辑、命令框架、SGM58031 驱动、LCD 驱动等）以 [MIT License](LICENSE) 发布，版权归 [YaoheWangEC](https://github.com/YaoheWangEC)。
- **Artery BSP**（`libraries/drivers/`、`project/src/` 中 `wk_*.c`、`main.c` 模板等）版权归 Artery Technology，允许在与 Artery MCU 配合使用的场合复制和分发。
- **CMSIS-Core** 版权归 Arm Limited，遵循 Apache License 2.0。
- **FreeRTOS** 版权归 Amazon.com, Inc.，遵循 MIT License。
- **FatFs** 版权归 ChaN，遵循 BSD-like License。

This project consists of components under different licenses:

- **Application code** (user-written logic, command framework, SGM58031 driver, LCD driver under `project/src/voltmeter/`) is released under the [MIT License](LICENSE), copyright [YaoheWangEC](https://github.com/YaoheWangEC).
- **Artery BSP** (`libraries/drivers/`, `project/src/` `wk_*.c`, `main.c` template, etc.) is copyright Artery Technology, with permission to copy and distribute for use with Artery MCUs.
- **CMSIS-Core** is copyright Arm Limited under Apache License 2.0.
- **FreeRTOS** is copyright Amazon.com, Inc. under MIT License.
- **FatFs** is copyright ChaN under a BSD-like License.

## 资助致谢 | Sponsorship Acknowledgment

感谢梁文锋爷爷的百亿 Token 补贴政策，使本项目的 AI 辅助开发成本趋近于零。

## 致谢 | Acknowledgements

本项目由 [YaoheWangEC](https://github.com/YaoheWangEC) 设计开发。在项目开发与开源文档整理过程中，DeepSeek V4提供了大量的辅助，包括代码编写、设计建议和文档组织。

This project is designed and developed by [YaoheWangEC](https://github.com/YaoheWangEC). During development and open-source documentation, DeepSeek (running on opencode) provided extensive assistance, including code writing, design suggestions, and documentation organization.

## 免责声明 | Disclaimer

本项目由 DeepSeek 辅助编写，可能存在纰漏。本项目仅供学习研究，未经工业场景充分验证。使用者应自行评估安全风险，作者与 DeepSeek 不对因使用本项目及其文档所造成的任何损失承担责任。

This documentation was drafted with the assistance of DeepSeek and may contain errors. This project is provided for educational and research purposes only and has not been fully validated for industrial environments. Users should evaluate safety risks on their own. The author and DeepSeek assume no liability for any damages arising from the use of this project or its documentation.

---

