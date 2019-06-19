/*
 * =============================================================================
 *
 *       Filename:  UDPServer.h
 *
 *    Description:  udpÇý¶¯
 *
 *        Version:  1.0
 *        Created:  2018-03-05 17:31:34 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _UDPSERVER_H
#define _UDPSERVER_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
#include <stdint.h>

#define MSG_SENDSUCCESS	 1		//wParam
#define MSG_SENDTIMEOUT	 2		//wParam

	typedef struct _SocketHandle {
		char IP[16];
		int Port;
	} SocketHandle;

	typedef struct _SocketPacket {
		int Size;
		char Data[1];
	} SocketPacket;

	typedef struct _UdpSocket {
		int handle;
		SocketHandle *ABinding;
		SocketPacket *AData;
	}UdpSocket;

	typedef void (*CallBackUDP)(int Ret,void *CallBackData);

	struct _UdpServerPriv;
	typedef struct _TUDPServer {
		struct _UdpServerPriv *priv;
		void (* Destroy)(struct _TUDPServer *This);
		int (* RecvBuffer)(struct _TUDPServer *This,void *buf,int count,int TimeOut,
				void * from,int * fromlen);
		int (* SendBuffer)(struct _TUDPServer *This,const char *IP,int port,const void *buf,int count);
		void (*AddTask)(struct _TUDPServer *This,const char *IP,int Port,void *pData,int Size,int Times,
				CallBackUDP Func,void *CallBackData);
        void (*KillTask)(struct _TUDPServer *This,const char *IP,int Port);
        int (*KillTaskCondition)(struct _TUDPServer* This,int (*condition)(void *arg1,void *arg2),void *arg);
        

        void (*udpSocketRead)(SocketHandle *ABinding,SocketPacket *AData);
	} TUDPServer;


	TUDPServer* udpServerCreate(int Port,const char *queue_name);
    void udpServerInit(void (*udpSocketRead)(SocketHandle *ABinding,SocketPacket *AData),
            int port);
    extern TUDPServer *udp_server;
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
