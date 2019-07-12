#ifndef TUDPClientH
#define TUDPClientH

#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------------

// 设置可靠发送的重发次数(含首次发送)为3次
#ifndef FALSE
    #define FALSE 0
    #define TRUE 1
    #define BOOL int
#endif

typedef struct _TUDPClient
{
#ifndef WIN32
	int m_socket;
#else
    unsigned m_socket;
#endif
    void (* Destroy)(struct _TUDPClient *This);
	void (* Clear)(struct _TUDPClient *This);
	int (* RecvBuffer)(struct _TUDPClient *This,void *buf,int count,int TimeOut,
        void * from,int * fromlen);
	int (* SendBuffer)(struct _TUDPClient *This,const char *IP,int port,const void *buf,int count);
	int (* TrusthSend)(struct _TUDPClient *This,const char *IP,int port,const void *buf,int count);
	int (* TrusthRecv)(struct _TUDPClient *This,void *buf,int count,int TimeOut);
} TUDPClient;

extern TUDPClient *udpclient;
//---------------------------------------------------------------------------

//  创建一个UDP客户端，Port为0则不绑定端口
TUDPClient* TUDPClient_Create(int Port);

#ifdef __cplusplus
}
#endif

#endif
