/*
 * =============================================================================
 *
 *       Filename:  uart.h
 *
 *    Description:  uart driver
 *
 *        Version:  1.0
 *        Created:  2016-08-06 16:49:21 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _UART_H
#define _UART_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define MAX_SEND_BUFF 128

	struct _UartServerPriv;
	typedef  struct _UartServer{
		struct _UartServerPriv *priv;
		int (*open)(struct _UartServer *This,int port,int baudrate);
		int (*send)(struct _UartServer *This,void *Buf,int datalen);
		//接收数据,直至5MS内没有数据返回
		int (*recvBuffer)(struct _UartServer *This,void *Buf,uint32_t BufSize);
		int (*waitFor)(struct _UartServer *This,unsigned int Ms);
		void (*clear)(struct _UartServer *This);
		void (*close)(struct _UartServer *This);
		void (*destroy)(struct _UartServer *This);

	}UartServer;

	extern UartServer *uartServerCreate(void (*func)(void));
	extern UartServer *uart;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
