#ifndef TLINK_SIP

#ifndef RTPObjectH
#define RTPObjectH

#include <stdint.h>
#include "udp_client.h"
//---------------------------------------------------------------------------

#define MAXENCODEPACKET (200*1024-16)

typedef struct {
	unsigned int		packet_cnt;		//分包数量
	unsigned int		packet_size;	//分包大小
	unsigned int		packet_idx;		//包索引
	unsigned int		alen;			//audio长度
	unsigned int		atype;
	unsigned int  		tlen;			//数据长度
	unsigned int		dead;
	unsigned int		seq;			//帧序号
	unsigned int  		slen;			//第一帧长度
	unsigned int  		vtype;			//第一帧类型
	unsigned int		checksum;		// 校验和
	unsigned char	 	sdata[MAXENCODEPACKET];	//帧数据
}rec_body;

#define RECBODYSIZE ((unsigned int)sizeof(rec_body))
#define RECHEADSIZE ((unsigned int)(sizeof(rec_body)-MAXENCODEPACKET))

typedef struct _TRTPObject
{
	unsigned int SendPacketIdx;
	int Terminate;
	int RecvTimeOut;
	TUDPClient *udp;
	char cPeerIP[16];
	char cMasterDevIP[16];
	char ViceDeviceIP[16];
	char cServerIP[16];
//	int ServerPort;
	unsigned int dwPeerIP;
	unsigned int dwMasterDevIP;
	unsigned int dwWEBSrvIP;
	uint8_t bRecvLocalPacket;			//是否接收到直传的包
	int PeerPort;
	int bInternet;		//是否基于互联网的传输
	uint8_t bTransBySrv;		//是否通过服务器中转包
	int LostFramePrecent;	//丢包率
	int DelayTime;			//延迟时间
	int SendPacketCnt;		//发送统计

	void (* Destroy)(struct _TRTPObject *This);
	int (* RecvBuffer)(struct _TRTPObject *This,void *buf,int count,int TimeOut);
	int (* SendBuffer)(struct _TRTPObject *This,void *buf,int count);
	int (* test)(struct _TRTPObject *This,void *buf,int count);
	void (*SetPeerPort)(struct _TRTPObject *This,int PeerPort);
	int (*SendPortNumber)(struct _TRTPObject *This,const char *IP,int Port,
		const char *SrvIP,unsigned int UserID);
} TRTPObject;
//---------------------------------------------------------------------------

//  创建一个UDP客户端，Port为0则不绑定端口
TRTPObject* TRTPObject_Create(const char *PeerIP,int PeerPort);

#endif

#endif
