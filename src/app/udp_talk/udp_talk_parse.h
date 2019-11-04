#ifndef TLINK_SIP

#ifndef RTPObjectH
#define RTPObjectH

#include <stdint.h>
#include "udp_client.h"
//---------------------------------------------------------------------------

#define MAXENCODEPACKET (200*1024-16)

typedef struct {
	unsigned int		packet_cnt;		//�ְ�����
	unsigned int		packet_size;	//�ְ���С
	unsigned int		packet_idx;		//������
	unsigned int		alen;			//audio����
	unsigned int		atype;
	unsigned int  		tlen;			//���ݳ���
	unsigned int		dead;
	unsigned int		seq;			//֡���
	unsigned int  		slen;			//��һ֡����
	unsigned int  		vtype;			//��һ֡����
	unsigned int		checksum;		// У���
	unsigned char	 	sdata[MAXENCODEPACKET];	//֡����
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
	uint8_t bRecvLocalPacket;			//�Ƿ���յ�ֱ���İ�
	int PeerPort;
	int bInternet;		//�Ƿ���ڻ������Ĵ���
	uint8_t bTransBySrv;		//�Ƿ�ͨ����������ת��
	int LostFramePrecent;	//������
	int DelayTime;			//�ӳ�ʱ��
	int SendPacketCnt;		//����ͳ��

	void (* Destroy)(struct _TRTPObject *This);
	int (* RecvBuffer)(struct _TRTPObject *This,void *buf,int count,int TimeOut);
	int (* SendBuffer)(struct _TRTPObject *This,void *buf,int count);
	int (* test)(struct _TRTPObject *This,void *buf,int count);
	void (*SetPeerPort)(struct _TRTPObject *This,int PeerPort);
	int (*SendPortNumber)(struct _TRTPObject *This,const char *IP,int Port,
		const char *SrvIP,unsigned int UserID);
} TRTPObject;
//---------------------------------------------------------------------------

//  ����һ��UDP�ͻ��ˣ�PortΪ0�򲻰󶨶˿�
TRTPObject* TRTPObject_Create(const char *PeerIP,int PeerPort);

#endif

#endif
