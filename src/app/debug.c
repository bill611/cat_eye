/*
 * =============================================================================
 *
 *       Filename:  UDPServer.c
 *
 *    Description:  udp驱动
 *
 *        Version:  1.0
 *        Created:  2018-03-05 17:33:54
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
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "thread_helper.h"
#include "debug.h"
#include "queue.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern void vTaskList( char * pcWriteBuffer  );

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#ifndef NELEMENTS

#define NELEMENTS(array)		/* number of elements in an array */ \
		(sizeof (array) / sizeof ((array) [0]))
#endif

#define MSG_SENDSUCCESS	 1		//wParam
#define MSG_SENDTIMEOUT	 2		//wParam


#define TASKTIMEOUT (200)//(30)			//300ms
#define MAXLIST 	(50)

#define UDP_PACKET_MAX 1000		//UDP包最大长度
#define UDP_DEBUG_PORT 1221 

#define SIZE_OF_STRING(x)  x,strlen(x)

#define UDP_PRINT(x)  \
		udp_debug->SendBuffer(udp_debug, ABinding->IP,UDP_DEBUG_PORT, \
				SIZE_OF_STRING(x)); \

#define UDP_SEND(x,len)  \
		udp_debug->SendBuffer(udp_debug, ABinding->IP,UDP_DEBUG_PORT, \
				x,len); \

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

typedef struct _LogData{
	char *cmd;
	void (*func)(SocketHandle *ABinding,SocketPacket *AData);
}LogData;

typedef void (*CallBackUDP)(int Ret,void *CallBackData);

struct _UdpServerPriv;
typedef struct _TUDPServer {
	struct _UdpServerPriv *priv;
	void (* Destroy)(struct _TUDPServer *This);
	int (* GetSocket)(struct _TUDPServer *This);
	int (* Terminated)(struct _TUDPServer *This);
	int (* RecvBuffer)(struct _TUDPServer *This,void *buf,int count,int TimeOut,
			void * from,int * fromlen);
	int (* SendBuffer)(struct _TUDPServer *This,const char *IP,int port,const void *buf,int count);
	void (*AddTask)(struct _TUDPServer *This,const char *IP,int Port,void *pData,int Size,int Times,
			unsigned int hWnd,CallBackUDP Func,void *CallBackData);
	void (*KillTask)(struct _TUDPServer *This,const char *IP,int Port);


	void (*udpSocketRead)(SocketHandle *ABinding,SocketPacket *AData);
} TUDPServer;


typedef struct _UdpThreadOps {
	unsigned int Handle;         //接收信息的窗口包柄
	int m_socket;       //套接字
	int port;           //端口号
	int Terminated;     //是否中止线程
}UdpThreadOps;

typedef struct _UdpSendLists {
	char IP[16];
	int Port;
	void *pData;
	int Size;
	int Times;
	int SendTimes;
	unsigned int hWnd;
	CallBackUDP Func;
	void *CallBackData;
} UdpSendLists;

typedef struct _UdpServerPriv {
	pthread_mutex_t mutex;		//队列控制互斥信号
	UdpSendLists Lists[MAXLIST];
	int ListCnt;
	struct _UdpThreadOps * control;
}UdpServerPriv;


/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static TUDPServer *udp_debug;
static Queue *udp_debug_queue;


/* ---------------------------------------------------------------------------*/
/**
 * @brief getDiffSysTick 计算32位差值
 *
 * @param new
 * @param old
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static uint32_t getDiffSysTick(uint64_t new,uint64_t old)
{
    uint32_t diff;
    if (new >= old)
        diff = new - old;
    else
        diff = 0XFFFFFFFF - old + new;
    return diff;
}

static char * getLocalIP(void)
{
	// return theConfig.ip;	
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpServerGetTickCount 返回系统tick
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static uint64_t udpServerGetTickCount(void)
{
	return getMs();  // 用此函数不能返回精准毫秒数
	// return SDL_GetTicks();
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpServerDelayMs 延时ms
 *
 * @param ms 毫秒
 */
/* ---------------------------------------------------------------------------*/
static void udpServerDelayMs(int ms)
{
	usleep(ms * 1000);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpServerPostMessage 发送侦听到的消息到各个窗口或线程
 *
 * @param handle 窗口句柄
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpServerPostMessage(int handle,
		SocketHandle *ABinding,
		SocketPacket *AData)
{
	UdpSocket socket_data;
	socket_data.handle = handle;
	socket_data.ABinding = ABinding;
	socket_data.AData = AData;
	udp_debug_queue->post(udp_debug_queue,&socket_data);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpServerTaskSend 发送task任务
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void udpServerTaskSend(TUDPServer* This,uint64_t *dwLastTick)
{
#if 0
	int i;
    uint64_t dwTick = udpServerGetTickCount();
    if(getDiffSysTick(dwTick,*dwLastTick) <= TASKTIMEOUT)
        return;
    *dwLastTick = dwTick;

	pthread_mutex_lock (&This->priv->mutex);		//加锁
	for(i=0;i<MAXLIST;i++) {
		UdpSendLists *pList = &This->priv->Lists[i];
		if(!pList->pData)
			continue;
		if(pList->SendTimes < pList->Times) {
			// 重发指定次数
			This->SendBuffer(This,pList->IP,pList->Port,pList->pData,pList->Size);
			pList->SendTimes++;
            // saveLog("times:%d,UDP Send IP:%s:%d,size:%d\n",pList->SendTimes,pList->IP,pList->Port,pList->Size );
		} else {
            // saveLog("send time out:%d,UDP Send IP:%s:%d,size:%d\n",pList->SendTimes,pList->IP,pList->Port,pList->Size );
			// 重发指定次数后失败
			// if(pList->Func)
				// pList->Func(MSG_SENDTIMEOUT,pList->CallBackData);
			free(pList->pData);
			pList->pData = NULL;
			This->priv->ListCnt--;
		}
	}
	pthread_mutex_unlock (&This->priv->mutex);		//解锁
#endif
}

//---------------------------------------------------------------------------
//  UDP Server监听数据线程
//---------------------------------------------------------------------------
static void udpServerRecvData(TUDPServer* This,
		SocketPacket *AData,
		struct sockaddr_in *from,
		int fromlen)
{
	SocketHandle *ABinding;
	UdpThreadOps *control = This->priv->control;

	ABinding = (SocketHandle*)calloc(1,sizeof(SocketHandle));
    if(ABinding == NULL) {
        free(AData);
		printf("udp server No Memory alloc ABinding\n");
        return;
    }
    memset(ABinding,0,sizeof(SocketHandle));
    ABinding->Port = htons(from->sin_port);
    char *pTmp = inet_ntoa(from->sin_addr);
	if(pTmp)
		strcpy(ABinding->IP,pTmp);
	udpServerPostMessage(control->Handle,ABinding,AData);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpServerThread udp服务线程
 *
 * @param ThreadData
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static void * udpServerThread(void *ThreadData)
{
//	socklen_t addrlen;
	TUDPServer *This = (TUDPServer *)ThreadData;
	UdpThreadOps *control = This->priv->control;
	struct timeval timeout;
	fd_set fdR;
	uint64_t dwLastTick = udpServerGetTickCount();
	while(!control->Terminated)
	{
		FD_ZERO(&fdR);
		FD_SET(control->m_socket, &fdR);
		timeout.tv_sec=0;
		timeout.tv_usec=50000;		//50ms
        // usleep(10000);
		switch (select(control->m_socket+1, &fdR,NULL, NULL, &timeout))
        {
            case -1:
                // saveLog("udp server select err!\n");
                // This->SendBuffer(This,Public.center_msg[0].IP,UDPSERVER_PORT,err,strlen(err));
                // goto error; 20180111 xb 不能退出，
                break;
            case 0: // 未收到数据，udp服务发送task任务，间隔时间 TASKTIMEOUT
                udpServerTaskSend(This,&dwLastTick);
                break;
            default:  // 收到数据，处理数据
                if(FD_ISSET(control->m_socket,&fdR)) {
                    struct sockaddr_in from;
                    SocketPacket *AData = (SocketPacket *)calloc(1,UDP_PACKET_MAX);
                    int fromlen = sizeof(struct sockaddr_in);
                    memset(&from,0,sizeof(from));
                    if(AData == NULL) {
                        // saveLog("UDP Server No Memory alloc\n");
                        break;
                    }
                    AData->Size=recvfrom(control->m_socket,
                            AData->Data,
                            UDP_PACKET_MAX-4,
							0,
                            // MSG_NOSIGNAL,
                            (struct sockaddr*)&from,
                            (socklen_t *)&fromlen);
                    if(AData->Size > 0) {
                        udpServerRecvData(This,AData,&from,fromlen);
                    } else {
                        free(AData);
                    }
                } else
                    udpServerDelayMs(1);
                //为防止接收阻塞发送任务，同时也继续发送task任务,
                udpServerTaskSend(This,&dwLastTick);
                break;
        }
	}
error:
	// saveLog("UDP Server Thread Exit\n");
	control->Terminated = 1;
	free(control);
	pthread_exit(NULL);
	return NULL;
}

static void udpServerDestroy(TUDPServer *This)
{
	This->priv->control->Terminated = 1;
	close(This->priv->control->m_socket);
	free(This);
}

static int udpServerGetSocket(TUDPServer *This)
{
	return This->priv->control->m_socket;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpServerSendBuffer 直接发送数据
 *
 * @param This
 * @param IP
 * @param port
 * @param pBuf
 * @param size
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int udpServerSendBuffer(TUDPServer *This,const char *IP,int port,const void *pBuf,int size)
{
    struct sockaddr_in *p;
	struct sockaddr addr;
	struct hostent *hostaddr;
    if(This->priv->control->m_socket<0) {
        // saveLog("m_socket:%d\n",This->priv->control->m_socket );
		return -1;
    }
    if(IP[0]==0 || port==0) {
        // saveLog("IP:%s:%d\n",IP,port );
		return -2;
    }
    memset(&addr,0,sizeof(addr));
	p=(struct sockaddr_in *)&addr;
    p->sin_family=AF_INET;
    p->sin_port=htons(port);
	if(IP[0]<'0' || IP[0]>'9') {
		hostaddr = gethostbyname(IP);
        if(!hostaddr) {
            // saveLog("ERR hostaddr,IP:%s\n",IP );
			return -3;
        }
		memcpy(&p->sin_addr,hostaddr->h_addr,hostaddr->h_length);
	} else {
		p->sin_addr.s_addr = inet_addr(IP);
		if(		   (p->sin_addr.s_addr == INADDR_NONE)
				&& (strcmp(IP,"255.255.255.255") != 0)   ) {
            // saveLog("ERR INADDR_NONE,IP:%s\n",IP );
			return -4;
		}
	}
	return sendto(This->priv->control->m_socket,
			(char*)pBuf,
			size,
			0,
			// MSG_NOSIGNAL,
			&addr,
			sizeof(struct sockaddr_in));
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpServerRecvBuffer 接收数据
 *
 * @param This
 * @param pBuf
 * @param size
 * @param TimeOut
 * @param from
 * @param fromlen
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int udpServerRecvBuffer(TUDPServer *This,void *pBuf,int size,int TimeOut,
						  void * from,int * fromlen)
{
	int iRet;
	struct timeval timeout;
	fd_set fdR;

	if(This->priv->control->m_socket<0)
		return -1;
	if(TimeOut<0) {
		iRet = recvfrom(This->priv->control->m_socket,
				(char*)pBuf,
				size,
				0,
				// MSG_NOSIGNAL,
				(struct sockaddr *)from,
				(socklen_t *)fromlen);
		return iRet;
	} else {
		FD_ZERO(&fdR);
		FD_SET(This->priv->control->m_socket, &fdR);
		timeout.tv_sec=TimeOut / 1000;
		timeout.tv_usec=(TimeOut % 1000)*1000;
		if(select(This->priv->control->m_socket+1, &fdR,NULL, NULL, &timeout)<=0)
            return -1;
		iRet = recvfrom(This->priv->control->m_socket,
				(char*)pBuf,
				size,
				0,
				// MSG_NOSIGNAL,
				(struct sockaddr *)from,
				(socklen_t *)fromlen);
    	return iRet;
    }
}
//---------------------------------------------------------------------------
static int udpServerTerminated(TUDPServer *This)
{
	return This->priv->control->Terminated;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpServerAddTask 通过添加任务发送数据
 *
 * @param This
 * @param IP
 * @param Port
 * @param pData
 * @param Size
 * @param Times
 * @param hWnd
 * @param Func
 * @param CallBackData
 */
/* ---------------------------------------------------------------------------*/
static void udpServerAddTask(TUDPServer* This,
		const char *IP,
		int Port,
		void *pData,
		int Size,
		int Times,
		unsigned int hWnd,
		CallBackUDP Func,
		void *CallBackData)
{
	int idx;
    while(This->priv->ListCnt > MAXLIST) {
        // saveLog("Server task out!! cnt:%d\n",This->priv->ListCnt );
		udpServerDelayMs(100);
    }
	pthread_mutex_lock (&This->priv->mutex);		//加锁
	//搜索空闲任务位置
	for(idx=0;idx<MAXLIST;idx++) {
		if(This->priv->Lists[idx].pData==NULL)
			break;
	}
	//添加任务
	if(idx<MAXLIST) {
		This->priv->Lists[idx].pData = calloc(1,Size);
		memcpy(This->priv->Lists[idx].pData,pData,Size);
		strcpy(This->priv->Lists[idx].IP,IP);
		This->priv->Lists[idx].Port = Port;
		This->priv->Lists[idx].Size = Size;
		This->priv->Lists[idx].SendTimes = 0;
		This->priv->Lists[idx].Times = Times;
		This->priv->Lists[idx].hWnd = hWnd;
		This->priv->Lists[idx].Func = Func;
		This->priv->Lists[idx].CallBackData = CallBackData;
		This->priv->ListCnt++;
        // saveLog("add task id:%d,cnt:%d\n",idx,This->priv->ListCnt );
    } else {
        // saveLog("ERR idx:%d\n",idx );
    }
	pthread_mutex_unlock (&This->priv->mutex);		//解锁
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpServerKillTask删除任务
 *
 * @param This
 * @param IP
 * @param Port
 */
/* ---------------------------------------------------------------------------*/
static void udpServerKillTask(TUDPServer* This,const char *IP,int Port)
{
	int i;
	pthread_mutex_lock (&This->priv->mutex);		//加锁
	for(i=0;i<MAXLIST;i++) {
		UdpSendLists *pList = &This->priv->Lists[i];
		if(pList->pData && strcmp(pList->IP,IP)==0 && pList->Port==Port) {
			free(pList->pData);
			pList->pData = NULL;
			This->priv->ListCnt--;
		}
	}
	pthread_mutex_unlock (&This->priv->mutex);		//解锁
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpServerCreate 创建一个线程，监听指定的端口
 *
 * @param Handle
 * @param Port
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static TUDPServer* udpServerCreate(unsigned int Handle,int Port)
{
	int ret;
	int opt = 1;
	int trueval = 1;
	struct sockaddr_in local_addr;
	pthread_mutexattr_t mutexattr;
    TUDPServer* This;
	This = (TUDPServer *)calloc(1,sizeof(TUDPServer));
    if(This==NULL) {
		// saveLog("alloc UDPServer memory failt!\n");
	   	goto error;
	}
	This->priv = (UdpServerPriv*)calloc(1,sizeof(UdpServerPriv));
    if(This->priv==NULL) {
		// saveLog("alloc udp priv memory fail!\n");
	   	goto error;
	}
	This->priv->control =(struct _UdpThreadOps*)calloc(1,sizeof(struct _UdpThreadOps));
    if(This->priv->control==NULL) {
		// saveLog("alloc control memory fail!\n");
	   	goto error;
	}
	This->priv->control->Handle = Handle;
	This->priv->control->port = Port;
	This->priv->control->Terminated = 0;

	pthread_mutexattr_init(&mutexattr);
	/* Set the mutex as a recursive mutex */
	pthread_mutexattr_settype(&mutexattr,
			   // PTHREAD_MUTEX_RECURSIVE);
			   PTHREAD_MUTEX_RECURSIVE_NP);

	/* create the mutex with the attributes set */
	pthread_mutex_init(&This->priv->mutex, &mutexattr);
	/* destroy the attribute */
	pthread_mutexattr_destroy(&mutexattr);

	memset(&local_addr,0,sizeof(local_addr));
	//初始化套接字
	This->priv->control->m_socket=socket(AF_INET,SOCK_DGRAM,0);
	if(This->priv->control->m_socket < 0) {
		// saveLog("UDP Server init socket failed!\n");
		goto error;
	}
	ret = setsockopt(This->priv->control->m_socket,SOL_SOCKET,SO_BROADCAST,(char*)&opt,sizeof(opt));
	if(ret!=0) {
		// saveLog("setsockopt server error %d\n",ret);
	}
	//绑定套接字
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(Port);
	local_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(This->priv->control->m_socket, (struct sockaddr *)&local_addr, sizeof(struct sockaddr))<0) {
		// saveLog("bind to port failed!\n");
        close(This->priv->control->m_socket);
		goto error;
	}

    //在程序关闭后可以立即使用该端口
    setsockopt(This->priv->control->m_socket,SOL_SOCKET,SO_REUSEADDR,&trueval,sizeof(trueval));

    This->Destroy = udpServerDestroy;
    This->GetSocket = udpServerGetSocket;
    This->Terminated = udpServerTerminated;
    This->RecvBuffer = udpServerRecvBuffer;
    This->SendBuffer = udpServerSendBuffer;
	This->AddTask = udpServerAddTask;
	This->KillTask = udpServerKillTask;
	createThread(udpServerThread,This);
	return This;
error:
    if(This && This->priv->control)
        free(This->priv->control);
    if(This)
        free(This);
    return NULL;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpSocketReadThread 侦听端口数据接收线程
 *
 * @param arg
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static void *udpSocketReadThread(void *arg)
{
	UdpSocket socket_data;
	// 阻塞状态 不需要延时
	while (1) {
		udp_debug_queue->get(udp_debug_queue,&socket_data);
        if (udp_debug->udpSocketRead)
            udp_debug->udpSocketRead(socket_data.ABinding,socket_data.AData);
		free(socket_data.ABinding);
		free(socket_data.AData);
	}
}


static void cmdConfig(SocketHandle *ABinding,SocketPacket *AData)
{
	char send_data[2048] = {0};
	// configDebugPrint(send_data);
	UDP_PRINT(send_data);
}
static void cmdLog(SocketHandle *ABinding,SocketPacket *AData)
{
	char send_data[2048] = {0};
	FILE *fd = fopen("log.txt","rb");
	if (fd) {
		do {
			int ret = fread(send_data,1,2048,fd);
			if (ret == 0)
				break;
			UDP_PRINT(send_data);
		} while(1);
		fclose(fd);
	}
}

static void cmdClear(SocketHandle *ABinding,SocketPacket *AData)
{
	char send_data[2048] = {0};
	FILE *fd = fopen("log.txt","w");
	if (fd)
		fclose(fd);
	UDP_PRINT("clear ok\n");
}
static void cmdVersion(SocketHandle *ABinding,SocketPacket *AData)
{
	char send_data[2048] = {0};
	sprintf(send_data,"Version :%s,%s\n",DEVICE_SVERSION,DEVICE_KVERSION);
	UDP_PRINT(send_data);
}
static void cmdReboot(SocketHandle *ABinding,SocketPacket *AData)
{
}
static void cmdResetBC(SocketHandle *ABinding,SocketPacket *AData)
{
}
static void cmdResetC(SocketHandle *ABinding,SocketPacket *AData)
{
}
static LogData log_data[] = {
	{"config",cmdConfig},
	{"log",cmdLog},
	{"clear",cmdClear},
	{"version",cmdVersion},
	{"reboot",cmdReboot},
	{"resetbc",cmdResetBC},
	{"resetc",cmdResetC},
};
static void cmdHelp(SocketHandle *ABinding,SocketPacket *AData)
{
	char send_data[2048] = {0};
	char cmd[32];
	unsigned int i;
	for (i=0; i<NELEMENTS(log_data); i++) {
		if (i == NELEMENTS(log_data) - 1)
			sprintf(cmd,"%s",log_data[i].cmd);
		else
			sprintf(cmd,"%s,",log_data[i].cmd);
		strcat(send_data,cmd);
	}
	strcat(send_data,"\n");
	UDP_PRINT(send_data);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief udpDebugHandle 调试具体业务,在此处更改
 *
 * @param ABinding
 * @param AData
 */
/* ---------------------------------------------------------------------------*/
static void udpDebugHandle(SocketHandle *ABinding,SocketPacket *AData)
{
	if (ABinding == NULL || AData == NULL)
		return;
	if (AData->Size == 0)
		return;
	// if (strcmp(ABinding->IP,getLocalIP()) == 0)
		// return;
	char *data = (char *)AData->Data;
	unsigned int i;
	for (i=0; i<NELEMENTS(log_data); i++) {
		if (strncasecmp(data,SIZE_OF_STRING(log_data[i].cmd)) == 0) {
			log_data[i].func(ABinding,AData);
			break;
		}
	}
	if (i == NELEMENTS(log_data)) {
		cmdHelp(ABinding,AData);
	}

}
/* ---------------------------------------------------------------------------*/
/**
 * @brief udpServerInit 初始化udp服务
 *
 * @param port 侦听端口
 */
/* ---------------------------------------------------------------------------*/
void udpDebugInit(void)
{
	udp_debug = udpServerCreate(0,UDP_DEBUG_PORT);
	udp_debug->udpSocketRead = udpDebugHandle;
	udp_debug_queue = queueCreate("udp_debug",QUEUE_BLOCK,sizeof(UdpSocket));

	createThread(udpSocketReadThread,NULL);
}

void saveLog(char *fmt, ...)
{
	FILE *log_fd;
	struct stat stat_buf;   
	stat("log.txt", &stat_buf) ; 
	// printf("log.txt size:%ld\n", stat_buf.st_size);

	va_list args;
	va_start(args, fmt);
	vfprintf(stdout,fmt,args);
	if (stat_buf.st_size > 1024 *1024)
		log_fd = fopen("log.txt","w");
	else
		log_fd = fopen("log.txt","ab+");
	if (log_fd) {
		char time_now[50]; 
		GetDate(time_now,sizeof(time_now));
		fprintf(log_fd,"[%s]",time_now);
		vfprintf(log_fd,fmt,args);
		fflush(log_fd); 
		fclose(log_fd);
	}
	va_end(args);
	configSync();
}

