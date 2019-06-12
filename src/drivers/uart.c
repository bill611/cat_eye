/*
 * =============================================================================
 *
 *       Filename:  uart.c
 *
 *    Description:  uart driver
 *
 *        Version:  1.0
 *        Created:  2016-08-06 16:45:01
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <pthread.h>

#include <termios.h>	//termios.tcgetattr(),tcsetattr

#include "hal_uart.h"
#include "uart.h"
#include "queue.h"
#include "debug.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void uartReceiveCreate(UartServer * This);
static void uartSendCreate(UartServer * This);
/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

typedef struct {
	uint32_t baudrate; // 波特率
	uint8_t data_bit; // 数据长度
	uint8_t parity;   // 校验位
	uint8_t stop_bit; // 停止位
}PortInfo ;

typedef struct _UartServerPriv {
	int32_t fd;
	uint8_t terminated;
	Queue *queue;
	pthread_mutex_t mutex;		//队列控制互斥信号
	PortInfo comparam;  // linux接口使用
}UartServerPriv;

typedef struct _UartSendBuf{
	int len;
	uint8_t data[MAX_SEND_BUFF];
} UartSendBuf;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static void (*call_back_func)(void);
UartServer *uart = NULL;

/* ---------------------------------------------------------------------------*/
/**
 * @brief uartOpen 打开串口
 *
 * @param This
 * @param com 串口编号
 * @param baudrate 波特率
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int uartOpen(UartServer *This,int com,int baudrate)
{
	This->priv->fd = halUartOpen(com,
			baudrate,
			This->priv->comparam.data_bit,
			This->priv->comparam.parity,
			This->priv->comparam.stop_bit, NULL);
	uartReceiveCreate(This);
	uartSendCreate(This);
	return This->priv->fd > 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief printfbuf 调试输出发送串口字节
 *
 * @param pbuf
 * @param size
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
#if 1
static void printfbuf(void *pbuf,int size)
{
	int i;
	unsigned char *pData = (unsigned char *)pbuf;
	DPRINT("[uart-->send,%d]",size);
	for(i=0;i<size;i++) {
		DPRINT("%02X ",pData[i]);
	}
	DPRINT("\n");
}
#else
#define printfbuf(pbuf,size)
#endif

/* ---------------------------------------------------------------------------*/
/**
 * @brief uartSend 发送数据
 *
 * @param This
 * @param Buf
 * @param datalen
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int uartSend(UartServer *This,void *Buf,int datalen)
{
	UartSendBuf send_buf;
	pthread_mutex_lock (&This->priv->mutex);		//加锁
	memset(&send_buf,0,sizeof(UartSendBuf));
	send_buf.len = datalen;
	memcpy(send_buf.data,Buf,datalen);
	This->priv->queue->post(This->priv->queue,
			&send_buf);
	pthread_mutex_unlock (&This->priv->mutex);		//解锁
	return 1;
}

//---------------------------------------------------------------------------
static int uartRecvBuffer(UartServer *This,void *pBuf,uint32_t size)
{
	int leave_size = size;
	while(leave_size) {
		int len = halUartRead(This->priv->fd,&((char*)pBuf)[size-leave_size],leave_size);
		if(len <= 0)
			break;
		leave_size -= len;
		usleep(20000);
	}
	return size-leave_size;
}
//---------------------------------------------------------------------------
static void uartClear(UartServer *This)
{
	int result;
	char cBuf[128];
	do {
		result = uartRecvBuffer(This,cBuf,sizeof(cBuf));
	} while(result>0);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief uartClose 暂时关闭串口
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void uartClose(UartServer *This)

{
	if(This->priv->fd) {
		close(This->priv->fd);
		This->priv->fd = 0;
		This->priv->terminated = 1;
	}
}

//---------------------------------------------------------------------------
static void * uartReceiveThead(UartServer *This)
{
	int fs_sel;
	fd_set fs_read;
	struct timeval tv_timeout;

	while(!This->priv->terminated){
		FD_ZERO(&fs_read);
		FD_SET(This->priv->fd,&fs_read);
		tv_timeout.tv_sec = 3;
		tv_timeout.tv_usec = 0;
		fs_sel = select(This->priv->fd+1,&fs_read,NULL,NULL,&tv_timeout);
		if(fs_sel>0 && FD_ISSET(This->priv->fd,&fs_read)) {
			if (call_back_func)
				call_back_func();
		}
	}
	pthread_exit(NULL);
}

static void * uartSendThead(UartServer *This)
{
	UartSendBuf send_buf;
	while(!This->priv->terminated){
		This->priv->queue->get(This->priv->queue, &send_buf);
		halUartWrite(This->priv->fd, send_buf.data, send_buf.len);
		printfbuf(send_buf.data,send_buf.len);
		usleep(500000);
	}
	pthread_exit(NULL);
}

static void uartReceiveCreate(UartServer * This)
{
	int ret;
	pthread_t id;
	pthread_attr_t  attr;
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&id,&attr,(void *)uartReceiveThead,This);
	if(ret)
		DPRINT("[%s pthread failt,Error code:%d\n",__FUNCTION__,ret);

	pthread_attr_destroy(&attr);
}
static void uartSendCreate(UartServer * This)
{
	int ret;
	pthread_t id;
	pthread_attr_t  attr;
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr,PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&id,&attr,(void *)uartSendThead,This);
	if(ret)
		DPRINT("[%s pthread failt,Error code:%d\n",__FUNCTION__,ret);

	pthread_attr_destroy(&attr);
}

static void uartDestroy(UartServer * This)
{
	uartClose(This);
	This->priv->terminated = 1;
	pthread_mutex_destroy (&This->priv->mutex);
	free(This);
}

UartServer *uartServerCreate(void (*func)(void))
{
	pthread_mutexattr_t mutexattr;
	UartServer *This = (UartServer *)calloc(1,sizeof(UartServer));
	This->priv = (UartServerPriv *)calloc(1,sizeof(UartServerPriv));

	pthread_mutexattr_init(&mutexattr);
	/* Set the mutex as a recursive mutex */
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);

	/* create the mutex with the attributes set */
	pthread_mutex_init(&This->priv->mutex, &mutexattr);
	/* destroy the attribute */
	pthread_mutexattr_destroy(&mutexattr);

	call_back_func = func;

	This->priv->queue =
		queueCreate("uart_socket",QUEUE_BLOCK,sizeof(UartSendBuf));

	// This->priv->comparam.baudrate = 38400;
	This->priv->comparam.data_bit = 8;
	This->priv->comparam.parity = 0;
	This->priv->comparam.stop_bit = 1;
	This->open = uartOpen;
	This->close= uartClose;
	This->send = uartSend;
	This->recvBuffer = uartRecvBuffer;
	This->clear = uartClear;
	This->destroy = uartDestroy;

	return This;
}

void uartInit(void(*func)(void))
{
	uart = uartServerCreate(func);
	uart->open(uart,1,9600);
}
