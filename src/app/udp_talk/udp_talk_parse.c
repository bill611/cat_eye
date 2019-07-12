#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#include "udp_talk_parse.h"
#include "udp_client.h"

extern unsigned int PacketID;

#define MAXMTU (1400)  //xb modify 20150821


unsigned int GetIPBySockAddr(const void *Data)
{
	struct sockaddr_in * addr = (struct sockaddr_in*)Data;
	return addr->sin_addr.s_addr;
}
unsigned int my_inet_addr(const char *IP)
{
	return inet_addr(IP);
}
//----------------------------------------------------------------------------
static void TRTPObject_SetPeerPort(TRTPObject *This,int PeerPort)
{
	This->PeerPort = PeerPort;
	//printf("Set 2 PeerPort=%d\n", PeerPort);
}
//---------------------------------------------------------------------------
static void TRTPObject_Destroy(TRTPObject *This)
{
	This->udp->Destroy(This->udp);
    free(This);
	This = NULL;
}

void DelayMs(int ms);
#define PACKETHEAD ((int)(sizeof(rec_body)-MAXENCODEPACKET))

//分包发送方式，将数据分为MAXMTU+PACKETHEAD长度的包进行发送
static int TRTPObject_SendBuffer(TRTPObject *This,void *pBuf,int size)
{
	int i,Cnt,Pos,Len;
	int PeerPort = This->PeerPort;
	char *pData = (char*)pBuf;

	Cnt = (size - PACKETHEAD + MAXMTU - 1) / MAXMTU;
	if(Cnt==0) {
		Cnt = 1;
	}

	rec_body *pbody = (rec_body*)pBuf;
	pbody->packet_cnt = Cnt;
	pbody->packet_size = MAXMTU;
	pbody->dead = 0;			//不通过服务器转发
	Pos = PACKETHEAD;
	for(i=0;i<Cnt;i++) {
		pbody->packet_idx = i;
		if(i) {
			memcpy(&pData[Pos-PACKETHEAD],pData,PACKETHEAD);
		}
		//发送数据长度
		Len = ((size-Pos)>MAXMTU?MAXMTU:(size-Pos))+PACKETHEAD;
		This->udp->SendBuffer(This->udp,This->cPeerIP,PeerPort,
				&pData[Pos-PACKETHEAD],Len);
		Pos+=MAXMTU;
	}
	return size;
}
//---------------------------------------------------------------------------
//分包接收方式,在本函数中处理接收所有分包及拼包的过程
//---------------------------------------------------------------------------
static int TRTPObject_RecvBuffer(TRTPObject *This,void *pBuf,int size,int TimeOut)
{
	int i,j,Cnt,Pos,Len;
    unsigned int PackID;
	char cTmpBuf[MAXMTU+PACKETHEAD],*pData;
	rec_body *pbody = (rec_body*)pBuf;
	char sockaddr[64];
	unsigned int RecvIP;
	int Size = sizeof(sockaddr);
	pData = (char*)pBuf;
	int PackSize = 0;

#if 0
	int count = 0;
	Pos = This->udp->RecvBuffer(This->udp,pBuf,size,TimeOut,sockaddr,&Size);
	if (pbody->slen)
		printf("[00]p_cnt:%02d,p_size:%d,p_idx:%02d,alen:%04d,atype:%d,tlen:%06d,dead:%d,seq:%04d,slen:%06d,vtype:%d,start\n",
				pbody->packet_cnt,pbody->packet_size,pbody->packet_idx,
				pbody->alen,pbody->atype,pbody->tlen,pbody->dead,pbody->seq,pbody->slen,pbody->vtype );
	if (pbody->packet_idx == 0) {
		count = pbody->packet_cnt;
	}
	for (i=1; i<count; i++) {
		Pos = This->udp->RecvBuffer(This->udp,pBuf,size,TimeOut,sockaddr,&Size);
		if (pbody->slen) {
			if (i == count - 1) {
				printf("[%02d]p_cnt:%02d,p_size:%d,p_idx:%02d,alen:%04d,atype:%d,tlen:%06d,dead:%d,seq:%04d,slen:%06d,vtype:%d,end\n",
						i,pbody->packet_cnt,pbody->packet_size,pbody->packet_idx,
						pbody->alen,pbody->atype,pbody->tlen,pbody->dead,pbody->seq,pbody->slen,pbody->vtype );

			} else {
				printf("[%02d]p_cnt:%02d,p_size:%d,p_idx:%02d,alen:%04d,atype:%d,tlen:%06d,dead:%d,seq:%04d,slen:%06d,vtype:%d   |\n",
						i,pbody->packet_cnt,pbody->packet_size,pbody->packet_idx,
						pbody->alen,pbody->atype,pbody->tlen,pbody->dead,pbody->seq,pbody->slen,pbody->vtype );
			}
			
			if (i != pbody->packet_idx)
				printf("--------------------\n");
			if (Pos < 0) {
				printf("err :%s\n", strerror(errno));
			}
		} else if (pbody->alen){
			i--;	
		}
		
	}
	return 0;
#endif
	//如果IP不对，丢弃该包
	for(i=0; i<10; i++) {
		Pos = This->udp->RecvBuffer(This->udp,pBuf,size,TimeOut,sockaddr,&Size);
		// printf("[start %d]p_cnt:%02d,p_size:%d,p_idx:%02d,alen:%04d,atype:%d,tlen:%06d,dead:%d,seq:%04d,slen:%06d,vtype:%d\n",
				// i,pbody->packet_cnt,pbody->packet_size,pbody->packet_idx,
				// pbody->alen,pbody->atype,pbody->tlen,pbody->dead,pbody->seq,pbody->slen,pbody->vtype );
		if(Pos <= 0) {
			//接收错误，由下面过程处理
			break;
		}
		RecvIP = GetIPBySockAddr(sockaddr);
		if(RecvIP == This->dwPeerIP)
			break;
	}
	if(i == 10) {
		printf("[%s]%d\n", __FUNCTION__,__LINE__);
		return 0;
	}

	//接收错误处理
	if(Pos<PACKETHEAD) {
		if(Pos<=0) {
			This->RecvTimeOut++;
			printf("[1]Rtp recv data timeout %d\n",This->RecvTimeOut);
			if(This->RecvTimeOut<10)
				return 0;
			return -1;
		} else {
			return 0;
		}
	}
	This->RecvTimeOut=0;
	//首包索引必须为0
	if(pbody->packet_idx!=0) {
		return 0;
	}


	//保存有多少个数据包
	Cnt = pbody->packet_cnt;
	PackID = pbody->seq;
	PackSize = pbody->packet_size;
	if(Cnt==1) {
		return Pos;
	}
	//接收剩余的分包
	pbody = (rec_body*)cTmpBuf;
	for(i=1;i<Cnt;i++) {
		//如果IP不对，丢弃该包
		for(j=0;j<10;j++) {
			Len = This->udp->RecvBuffer(This->udp,cTmpBuf,MAXMTU+PACKETHEAD,TimeOut,sockaddr,&Size);
			// printf("[continue %02d]p_cnt:%02d,p_size:%d,p_idx:%02d,alen:%04d,atype:%d,tlen:%06d,dead:%d,seq:%04d,slen:%06d,vtype:%d\n",
					// i,pbody->packet_cnt,pbody->packet_size,pbody->packet_idx,
					// pbody->alen,pbody->atype,pbody->tlen,pbody->dead,pbody->seq,pbody->slen,pbody->vtype );
			if(Len<=0) {
				break;
			}
			unsigned int RecvIP2 = GetIPBySockAddr(sockaddr);

			if(RecvIP2 == RecvIP) {
				break;
			}
		}
		if(j==10)
			Len = 0;

		if(Len<=0) {
			This->RecvTimeOut++;
			printf("[2]Rtp recv data timeout %d\n",This->RecvTimeOut);
			if(This->RecvTimeOut<10)
				return 0;
			return -1;
		}

		if(pbody->seq!=PackID) {				//帧号错误
			printf("[%s]%d,pbody->seq:%d,pckid:%d\n", 
					__FUNCTION__,__LINE__,
					pbody->seq,PackID );
			return 0;
		}
		if(pbody->packet_idx>=Cnt) {			//分包索引错误
			printf("[%s]%d\n", __FUNCTION__,__LINE__);
			return 0;
		}
		// 中途接收分包遗漏，则接收完剩余分包后退出，丢弃该帧
		if ((pbody->packet_idx+1 == pbody->packet_cnt)
				&& (i != pbody->packet_idx)) {
			return 0;
		}


		if(Len>PACKETHEAD)
			memcpy(&pData[PACKETHEAD+pbody->packet_idx*MAXMTU],&cTmpBuf[PACKETHEAD],Len-PACKETHEAD);
		Pos+=Len-PACKETHEAD;
	}
	return Pos;
}
//---------------------------------------------------------------------------
//  创建一个UDP客户端，Port为0则不绑定端口
//---------------------------------------------------------------------------
TRTPObject* TRTPObject_Create(
	const char *PeerIP,
	int PeerPort)
{
    TRTPObject* This;

    This = (TRTPObject *)malloc(sizeof(TRTPObject));
    if(This==NULL) {
        return NULL;
    }
	memset(This,0,sizeof(TRTPObject));

	strcpy(This->cPeerIP,PeerIP);
	This->udp = TUDPClient_Create(8800);
	if(This->udp == NULL) {
		free(This);
        return NULL;
	}


	This->dwPeerIP = my_inet_addr(PeerIP);
	This->DelayTime = 10;
	This->PeerPort = PeerPort;
	This->Destroy = TRTPObject_Destroy;
    This->RecvBuffer = TRTPObject_RecvBuffer;
    This->SendBuffer = TRTPObject_SendBuffer;
	This->SetPeerPort = TRTPObject_SetPeerPort;
    return This;

}
