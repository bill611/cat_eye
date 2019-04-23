/*
 * =============================================================================
 *
 *       Filename:  hal_uart.h
 *
 *    Description:  硬件层 串口驱动接口
 *
 *        Version:  1.0
 *        Created:  2018-12-13 08:45:09 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _HAL_UART_H
#define _HAL_UART_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>

	/* ---------------------------------------------------------------------------*/
	/**
	 * @brief halUartOpen 硬件层 打开串口
	 *
	 * @param com 串口编号 
	 * @param baudrate 波特率
	 * @param data_bit 数据长度
	 * @param parity   校验位
	 * @param stop_bit 停止位
	 * @param callback_func 接收回调函数，无则传NULL
	 *
	 * @returns 
	 */
	/* ---------------------------------------------------------------------------*/
	int halUartOpen(int com,
			int baudrate,
			uint8_t data_bit,
			uint8_t parity,
			uint8_t stop_bit,
			void *callback_func);
	/* ---------------------------------------------------------------------------*/
	/**
	 * @brief halUartRead 硬件层 串口读取
	 *
	 * @param fd 串口句柄
	 * @param buf 读出buf存储
	 * @param size 读出字节
	 *
	 * @returns 实际读出字节
	 */
	/* ---------------------------------------------------------------------------*/
	int halUartRead(int fd,void *buf,uint32_t size);
	/* ---------------------------------------------------------------------------*/
	/**
	 * @brief halUartWrite 硬件层 串口写入
	 *
	 * @param fd 串口句柄
	 * @param buf 写入buf存储
	 * @param size 写入字节大小
	 *
	 * @returns 实际写入字节大小
	 */
	/* ---------------------------------------------------------------------------*/
	int halUartWrite(int fd,void *buf,uint32_t size);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
