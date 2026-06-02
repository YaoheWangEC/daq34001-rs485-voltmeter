#ifndef __COMMAND_IO_H__
#define __COMMAND_IO_H__

#include <stdint.h>
#include <stdbool.h>
#include "command_fifo.h"

#define COMMAND_IO_RX_BUF_SIZE  (CMD_MAX_LENGTH + 4)
#define COMMAND_IO_TX_BUF_SIZE  COMMAND_IO_RX_BUF_SIZE

/**
  * @brief  初始化命令 IO 模块
  *
  * 清零缓冲区与状态, 初始化 RS485 为接收模式,
  * 配置 DMA RX 通道 (normal 模式) 并使能 IDLE 中断。
  */
void command_io_init(void);

/**
  * @brief  从接收缓冲区取出一个字节
  * @param  c 指向存放读取结果的指针
  * @retval true  成功读取
  * @retval false 接收缓冲区为空
  */
bool command_io_get_char(uint8_t *c);

/**
  * @brief  向发送缓冲区压入一个字节
  * @param  c 待发送的字节
  * @retval true  压入成功
  * @retval false 发送缓冲区已满
  * @note   仅缓冲数据, 需调用 command_io_transmit_start() 启动发送。
  */
bool command_io_put_char(uint8_t c);

/**
  * @brief  向发送缓冲区压入一个字符串
  * @param  str 待发送的字符串 (以 \\0 结尾)
  * @retval true  压入成功
  * @retval false 发送缓冲区剩余空间不足, 整体丢弃
  * @note   原子操作: 要么整体压入, 要么整体丢弃, 不会部分写入。
  */
bool command_io_put_string(const char *str);

/**
  * @brief  向发送缓冲区压入指定长度的数据
  * @param  buf 待发送数据指针
  * @param  len 数据长度 (字节)
  * @retval true  压入成功
  * @retval false 发送缓冲区剩余空间不足, 整体丢弃
  * @note   与 command_io_put_string 不同, 本函数不依赖 \0 结尾,
  *         适合超长响应分帧发送场景。原子操作: 要么整体压入, 要么整体丢弃。
  */
bool command_io_put_buf(const char *buf, uint32_t len);

/**
  * @brief  查询 TX 是否空闲
  * @retval true  发送正在进行中
  * @retval false 发送已完成, 可启动下一次发送
  */
bool command_io_is_tx_busy(void);

/**
  * @brief  启动 DMA 发送 (非阻塞)
  * @retval true  发送已启动
  * @retval false 发送缓冲区为空, 或上一次发送尚未完成
  *
  * 将 tx_data_buf 拷贝到 DMA 缓冲区, 切换 TX 模式,
  * 配置并启动 DMA TX 通道, 使能 DMA 传输完成中断。
  * 发送完成后 ISR 自动拉低 DE 并置 tx_cplt = true。
  */
bool command_io_transmit_start(void);

/**
  * @brief  清空收发缓冲区 (丢弃所有未处理数据)
  */
void command_io_flush(void);

/**
  * @brief  从命令 FIFO 中取出一条完整命令
  * @param  buf 用于存放命令字符串的缓冲区
  * @param  len 返回命令长度 (含 \0 结束符)
  * @retval true  成功取出
  * @retval false FIFO 为空
  */
bool command_io_pop(char *buf, uint16_t *len);

/**
  * @brief  USART RX 中断处理 (IDLE / DMA full)
  *
  * 由 USART 的 IDLE 中断和 RX DMA 满中断调用。
  * 将 DMA 硬件缓冲区数据拷入 rx_data_buf,
  * 检测 \r\n 命令结束符后压入命令 FIFO, 最后重启 DMA RX。
  */
void command_io_rx_handler(void);

/**
  * @brief  DMA TX 完成中断处理
  *
  * 由 USART TX DMA 中断调用。
  * 等待 USART 移位寄存器全部发送完毕 (TDC 标志),
  * 回到接收模式, 置 tx_cplt = true。
  */
void command_io_tx_handler(void);

#endif /* __COMMAND_IO_H__ */
