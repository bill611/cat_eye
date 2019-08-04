/*
 * =============================================================================
 *
 *       Filename:  tcp_client.c
 *
 *    Description:  TCP客户端
 *
 *        Version:  1.0
 *        Created:  2019-06-18 13:41:58
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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#include "tcp_client.h"
#include "debug.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define TIME_OUT_TIME 3  //connect超时时间

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
TTCPClient* tcp_client;

//---------------------------------------------------------------------------
static void TTCPClient_Destroy(TTCPClient *This) {
	if(This->m_socket>0)
    	This->DisConnect(This);
    free(This);
}
//---------------------------------------------------------------------------
static int TTCPClient_Connect(struct _TTCPClient *This,const char *IP,int port,int TimeOut)
{
    struct sockaddr_in *p;
	struct sockaddr addr;
    unsigned long ul = 1;
	int ret = 0;

    if(This->m_socket)
        This->DisConnect(This);

	memset(&addr,0,sizeof(addr));
	p=(struct sockaddr_in *)&addr;
    p->sin_family=AF_INET;
    p->sin_port=htons(port);
    if(inet_aton(IP,&p->sin_addr)<0)
    {
		DPRINT("Can't know IP address as %s",IP);
        return -1;
    }
	This->m_socket=socket(PF_INET,SOCK_STREAM,0);
	if(This->m_socket==0) {
		DPRINT("TCP Client init socket failed!\n");
        return -1;
    }

	if( connect(This->m_socket, &addr,sizeof(addr)) == -1) {
		int error=-1, len;
		struct timeval tm;
		fd_set set;

		tm.tv_sec  = TimeOut/1000;
		tm.tv_usec = (TimeOut%1000)*1000;

		FD_ZERO(&set);
		FD_SET(This->m_socket, &set);
		if( select(This->m_socket+1, NULL, &set, NULL, &tm) > 0) {
			getsockopt(This->m_socket, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
			if(error == 0) 
				ret = 1;
			else  {
				perror("Err: select()");
			}
		} else  {
			perror("Err: connect()");
		}
	} else 
		ret = 1;

	ul = 0;
	ioctl(This->m_socket, FIONBIO, &ul);  //设置为阻塞模式

	if(ret==0) {
        This->DisConnect(This);
		return -1;
	}
	return 0;
}
//---------------------------------------------------------------------------
static void TTCPClient_DisConnect(struct _TTCPClient *This)
{
	if (This->m_socket)
		close(This->m_socket);
    This->m_socket = 0;
}
//---------------------------------------------------------------------------
static int TTCPClient_SendBuffer(TTCPClient *This,const void *pBuf,int size)
{
    return send(This->m_socket,pBuf,size,MSG_NOSIGNAL);
}
//---------------------------------------------------------------------------
static int TTCPClient_RecvBuffer(TTCPClient *This,void *pBuf,int size,int TimeOut)
{
	struct timeval timeout;
	fd_set fdR;
	int LeaveSize = size;

	if(This->m_socket<=0)
		return -1;
	if(TimeOut<0) {
		while(LeaveSize) {
			int RecvSize = recv(This->m_socket,((char*)pBuf)+(size-LeaveSize),LeaveSize,MSG_NOSIGNAL);
			if(RecvSize<=0)
				break;
			LeaveSize-=RecvSize;
		}
    	return size-LeaveSize;
	}
    else
    {
		while(LeaveSize) {
			int RecvSize;
			FD_ZERO(&fdR);
			FD_SET(This->m_socket, &fdR);
			timeout.tv_sec=TimeOut / 1000;
			timeout.tv_usec=(TimeOut % 1000) * 1000;
			if(select(This->m_socket+1, &fdR,NULL, NULL, &timeout)<=0 || !FD_ISSET(This->m_socket,&fdR)) {
				// printf("tcpclient:read select timeout\n");
				return -1;
			}
    		RecvSize=recv(This->m_socket,((char*)pBuf)+(size-LeaveSize),LeaveSize,MSG_NOSIGNAL);
			if(RecvSize<=0)
				break;
			LeaveSize-=RecvSize;
		}
    	return size-LeaveSize;
    }
}
//---------------------------------------------------------------------------
//  创建一个UDP客户端，Port为0则不绑定端口
//---------------------------------------------------------------------------
TTCPClient* TTCPClient_Create(void)
{
    TTCPClient* This;

    This = (TTCPClient *)malloc(sizeof(TTCPClient));
    if(This==NULL) {
        DPRINT("alloc TCPClient memory failt!\n");
        return NULL;
    }
    This->Destroy = TTCPClient_Destroy;
    This->RecvBuffer = TTCPClient_RecvBuffer;

    This->SendBuffer = TTCPClient_SendBuffer;

    This->Connect = TTCPClient_Connect;

    This->DisConnect = TTCPClient_DisConnect;

    return This;
}
void tcpClientInit(void)
{
	tcp_client = TTCPClient_Create();
}
