/*
 * =============================================================================
 *
 *       Filename:  tcp_client.h
 *
 *    Description:  TCP客户端
 *
 *        Version:  1.0
 *        Created:  2019-06-18 13:40:50 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _TCP_CLIENT_H
#define _TCP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	typedef struct _TTCPClient {
		int m_socket;
		void (* Destroy)(struct _TTCPClient *This);
		int (* Connect)(struct _TTCPClient *This,const char *IP,int port,int TimeOut);
		void (*DisConnect)(struct _TTCPClient *This);
		int (* RecvBuffer)(struct _TTCPClient *This,void *buf,int count,int TimeOut);
		int (* SendBuffer)(struct _TTCPClient *This,const void *buf,int count);
	} TTCPClient;

	//  创建一个UDP客户端，Port为0则不绑定端口
	TTCPClient* TTCPClient_Create(void);
	extern TTCPClient* tcp_client;
	void tcpClientInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
