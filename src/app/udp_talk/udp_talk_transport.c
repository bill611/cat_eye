/*
 * =============================================================================
 *
 *       Filename:  Rtp.c
 *
 *    Description:  传输处理
 *
 *        Version:  1.0
 *        Created:  2016-03-01 14:27:57
 *       Revision:  1.0
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <linux/fb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/time.h>
#include <dirent.h>
#include <pthread.h>

#include "udp_talk_protocol.h"
#include "udp_talk_transport.h"
#include "udp_talk_parse.h"
#include "share_memory.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#if DBG_RTP > 0
	#define DBG_P( x... ) printf( x )
#else
	#define DBG_P( x... )
#endif

#define AUDIO_SIZE		1024

#define LOADFUNC(func) \
do {\
	if (in->func)\
		priv->func = in->func;\
} while (0)

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
Rtp * udp_talk_trans = NULL;
static uint8_t RTPTerminate;		//RTP线程是否结束
static uint8_t SPITerminate;		//SPI线程是否结束
static uint8_t RTPThreadExit;		//RTP线程是否结束
static uint8_t SPIThreadExit;		//SPI线程是否结束
static TRTPObject *RTPObject = NULL;	//RTP传输对象
static PShareMemory RtpDcMem;          //RTP与Dc的共享内存

// 打印帧率
int seq = 0;
int cnt_test = 0;
static pthread_t RTP_pthread;
static pthread_t Encode_pthread;
static pthread_t SPI_pthread;
static void *call_back_data;
static int last_i_seq = 0;
static int last_seq = 0;

static int isIframe(unsigned char *sdata)
{
	if (       (sdata[0] == 0x00)
			&& (sdata[1] == 0x00)
			&& (sdata[2] == 0x00)
			&& (sdata[3] == 0x01)
			&& (   (sdata[4] == 0x65) || (sdata[4] == 0x67)
				|| (sdata[4] == 0x25) || (sdata[4] == 0x27)) ) {
			return 1;
	} else if ((sdata[0] == 0x00)
			&& (sdata[1] == 0x00)
			&& (sdata[2] == 0x01)
			&& (   (sdata[3] == 0x65) || (sdata[3] == 0x67)
				|| (sdata[3] == 0x25) || (sdata[3] == 0x27)) ) {
			return 1;
	}
	return 0;
}

int needDecode(int cur_seq,unsigned char *sdata)
{
    //关键帧，需要解码
	if (isIframe(sdata)) {
		last_i_seq = cur_seq;
		last_seq = cur_seq;
		return 1;
	}
	if(cur_seq == (last_seq + 1)) {
		last_seq = cur_seq;
		return 1;
	}
	printf("loss seq:%d last_seq:%d last_i_seq:%d\n",cur_seq,last_seq,last_i_seq);
	return 0;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief decodeAudio 解码音频
 *
 * @param This
 * @param pbody 音频数据结构
 */
/* ---------------------------------------------------------------------------*/
static void decodeAudio(Rtp* This,rec_body *pbody)
{
	//处理音频
	if(pbody->alen <= 0 || pbody->alen >= 12000) { //保存远程衰减强度
		printf("Error! pbody->alen:%d\n",pbody->alen);
		return;
	}
	if (This->silence == FALSE) {
		if (This->interface->receiveAudio)
			This->interface->receiveAudio(This,pbody->sdata,pbody->alen);
	}
}
static void decodeVideo(Rtp* This,rec_body *pbody,int size)
{
	char *pBuf = NULL;
	if(RtpDcMem)
		RtpDcMem->WriteCnt(RtpDcMem);
	if(RtpDcMem)
		pBuf = RtpDcMem->SaveStart(RtpDcMem);//取缓冲区用于储存
	if(pBuf)
		memcpy(pBuf,pbody,size);
	if(RtpDcMem)
		RtpDcMem->SaveEnd(RtpDcMem,size);
}

static int rtpGetVideo(Rtp* This,void *data)
{
	rec_body *pbody = NULL;
	int size = 0;
	if (RtpDcMem == NULL)
		return 0;
	if (RtpDcMem->WriteCnt(RtpDcMem) <= 0)
		return 0;
	pbody = (rec_body *)RtpDcMem->GetStart(RtpDcMem,&size);
	if (!pbody || size == 0) {
		return 0;
	}
	if (pbody->slen == 0) {
		RtpDcMem->GetEnd(RtpDcMem);
		return 0;
	}
	if(needDecode(pbody->seq,pbody->sdata) == 0) {
		RtpDcMem->GetEnd(RtpDcMem);
		return 0;

	}
	memcpy(data,pbody->sdata,pbody->slen);
	// printf("[%s]%d\n", __func__,pbody->slen);
	RtpDcMem->GetEnd(RtpDcMem);
	return pbody->slen;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief rtpThreadExecute RTP线程，接收音频,视频数据
 *
 * @param ThreadData
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static void * rtpThreadExecute(void *ThreadData)
{
	Rtp* This = (Rtp*)ThreadData;
	char *pData;
	int Size=0;

	pData = (char*)malloc(sizeof(rec_body));
	if(pData == NULL) {
		printf("Can't alloc memory!\n");
		RTPThreadExit = TRUE;
		pthread_exit(NULL);
		return NULL;
	}
	while(!RTPTerminate) {
		memset(pData,0,sizeof(rec_body));
		if(RTPObject != NULL) {
			Size = RTPObject->RecvBuffer(RTPObject,pData,sizeof(rec_body),500);
		}
		if(Size == -1) {
			//对方已关闭连接
			printf("RTP receive is abort!\n");
			if(RtpDcMem) {
				RtpDcMem->SaveEnd(RtpDcMem,0);
			}
			This->interface->abortCallBack(This);
			break;
		} else if(Size > 0) {
			rec_body *pbody = (rec_body*)pData;
			if(pbody->alen > 0) { //解码音频
				decodeAudio(This,pbody);
			} else if (pbody->slen > 0){
				decodeVideo(This,pbody,Size);
			}
		}
		usleep(1000);
	}

	if (This->interface->receiveEnd)
		This->interface->receiveEnd(This);

	free(pData);
	pData = NULL;
	RTPThreadExit = TRUE;
	pthread_exit(NULL);
	return NULL;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief rtpThreadCreate 创建RTP线程
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void rtpThreadCreate(Rtp* This)
{
	int result;
	struct sched_param SchedParam;			//优先级
	pthread_attr_t threadAttr1;				//线程属性

	RTPTerminate = FALSE;
	RTPThreadExit = FALSE;

	pthread_attr_init(&threadAttr1);		//附加参数

	SchedParam.sched_priority = 3;
	pthread_attr_setschedparam(&threadAttr1, &SchedParam);
	pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);	//设置线程为自动销毁
	result = pthread_create(&RTP_pthread,&threadAttr1,rtpThreadExecute,This);
	if(result) {
		RTPThreadExit = TRUE;
		printf("rtpThreadCreate() pthread failt,Error code:%d\n",result);
	}
	pthread_attr_destroy(&threadAttr1);		//释放附加参数
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief spiAudio 发送音频
 *
 * @param This
 * @param pAudioPack
 *
 * @returns 0失败 1成功
 */
/* ---------------------------------------------------------------------------*/
static int spiAudio(Rtp* This,
		rec_body *pAudioPack)
{
	if(This->bTransAudio == 0 || !pAudioPack) {
		return 0;
	}
	int packsize = 0;
	short AudioBuf[AUDIO_SIZE];
	if (This->interface->sendAudio)
		packsize = This->interface->sendAudio(This,AudioBuf,AUDIO_SIZE);
	if(packsize <= 0) {
		return 1;
	}
	pAudioPack->seq++;
	pAudioPack->vtype = 0;
	pAudioPack->slen = 0;

	memcpy(pAudioPack->sdata, AudioBuf, packsize);
	pAudioPack->alen = packsize;
	pAudioPack->tlen = RECHEADSIZE+pAudioPack->alen;
	if(RTPObject) {
		RTPObject->SendBuffer(RTPObject,pAudioPack,pAudioPack->tlen);
	}
	return 1;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief spiHeart 没有音频发送时发送心跳包
 *
 * @param pAudioPack
 */
/* ---------------------------------------------------------------------------*/
static void spiHeart(rec_body *pAudioPack)
{
	pAudioPack->seq = 0;
	pAudioPack->vtype = 0;
	pAudioPack->slen = 0;
	pAudioPack->alen = 0;
	pAudioPack->tlen = RECHEADSIZE+pAudioPack->alen+32;
	if(RTPObject) {
		RTPObject->SendBuffer(RTPObject,pAudioPack,pAudioPack->tlen);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief spiThreadExecute 执行SPI，发送音频和视频
 *
 * @param ThreadData
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static void * spiThreadExecute(void *ThreadData)
{
	Rtp* This = (Rtp*)ThreadData;

	rec_body *pAudioPack = (rec_body*)malloc(sizeof(rec_body));
	seq=0;
	// 帧率 fps
	while(!SPITerminate) {
		if (spiAudio(This,pAudioPack) == 0) {
			spiHeart(pAudioPack);
		}
		usleep(5000);
	}
	seq = 0;
	cnt_test = 0;
	if (This->interface->sendEnd)
		This->interface->sendEnd(This);
	if(pAudioPack)
		free(pAudioPack);
	pAudioPack = NULL;
	SPIThreadExit = TRUE;
	pthread_exit(NULL);
	return NULL;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief spiThreadCreate 创建SPI线程
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void spiThreadCreate(Rtp* This)
{
	int result;
	pthread_attr_t threadAttr1;				//线程属性
	struct sched_param SchedParam;	//优先级

	SPITerminate = FALSE;
	SPIThreadExit = FALSE;

	SchedParam.sched_priority = 6;
	pthread_attr_init(&threadAttr1);

	pthread_attr_setdetachstate(&threadAttr1,PTHREAD_CREATE_DETACHED);	//设置线程为自动销毁
	result = pthread_create(&SPI_pthread,&threadAttr1,spiThreadExecute,This);
	if(result) {
		SPIThreadExit = TRUE;
		printf("spiThreadCreate() pthread failt,Error code:%d\n",result);
	}
	pthread_attr_destroy(&threadAttr1);		//释放附加参数
}


/* ---------------------------------------------------------------------------*/
/**
 * @brief rtpInit 初始化RTP连接
 *
 * @param This
 * @param IP 呼叫IP
 * @param Port
 *
 * @returns CALL_FAIL 失败 CALL_OK 成功
 */
/* ---------------------------------------------------------------------------*/
static int rtpInit(Rtp *This, const char *dest_ip, int Port)
{
	if(RTPObject == NULL) {
		RTPObject = TRTPObject_Create(dest_ip,Port);
	}

	if(RTPObject == NULL) {
		return CALL_FAIL;
	}
	return CALL_OK;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief rtpBuildConnect 建立RTP连接
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void rtpBuildConnect(Rtp *This)
{
	This->fpAudio = -1;
	This->bTransAudio = FALSE;
	This->bTransVideo = TRUE;
	if (This->interface->start)
		This->interface->start(This);
	if(RtpDcMem==NULL)
		RtpDcMem = CreateShareMemory(sizeof(rec_body),2,0);
	last_i_seq = 0;
	last_seq = 0;
	rtpThreadCreate(This);
	spiThreadCreate(This);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief rtpStartAudio 使能传输音频
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void rtpStartAudio(Rtp *This)
{
	return;
	This->bTransAudio = TRUE;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief rtpGetFpAudio 返回通话音频句柄地址
 *
 * @param This
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int *rtpGetFpAudio(Rtp *This)
{
	return &This->fpAudio;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief rtpSetSilence 设置通话时本地是否静音，
 * 用于通话时同时播放MP3情况
 *
 * @param This
 * @param value TRUE 静音 FALSE 非静音
 */
/* ---------------------------------------------------------------------------*/
static void rtpSetSilence(Rtp *This,int value)
{
	This->silence = value;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief rtpGetSilence 获得当前静音状态
 *
 * @param This
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int rtpGetSilence(Rtp *This)
{
	return This->silence;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief rtpSetPeerIp 设置通讯IP
 *
 * @param This
 * @param ip
 */
/* ---------------------------------------------------------------------------*/
static int rtpSetPeerIp(Rtp *This,char *ip)
{
    if (!RTPObject)
        return FALSE;
	strncpy(RTPObject->cPeerIP,ip,15);
	RTPObject->dwPeerIP = inet_addr(ip);
    return TRUE;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief rtpClose 通话结束，关闭RTP线程
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void rtpClose(Rtp *This)
{
	This->silence = FALSE;
	This->bTransAudio = FALSE;
	This->bTransVideo = FALSE;

	printf("Wait RTPThreadExit\n");
	RTPTerminate = TRUE;
	while(!RTPThreadExit) {usleep(10000);}

	if(RtpDcMem) {
		RtpDcMem->CloseMemory(RtpDcMem);
	}

	printf("Wait SPIThreadExit\n");
	SPITerminate = TRUE;
	while(!SPIThreadExit) {usleep(10000);}

	if(RtpDcMem) {
		RtpDcMem->Destroy(RtpDcMem);
		RtpDcMem = NULL;
	}

	if(RTPObject) {
		RTPObject->Destroy(RTPObject);  //bug maybe ---Jack
		RTPObject = NULL;
	}
	printf("[%s]\n", __func__);

}
static void rtpDestroy(Rtp **This)
{
	free(*This);
	*This = NULL;
}
static void loadInterface(UdpTalkTransInterface *priv,UdpTalkTransInterface *in)
{
	LOADFUNC(abortCallBack);
	LOADFUNC(start);
	LOADFUNC(receiveAudio);
	LOADFUNC(receiveEnd);
	LOADFUNC(sendStart);
	LOADFUNC(sendVideo);
	LOADFUNC(sendAudio);
	LOADFUNC(sendEnd);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief createRtp 创建RTP传输对象
 *
 * @param h264_type_tmp 视频分辨率结构
 * @param abortCallBack 视频中断时回调函数
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
Rtp * createRtp(UdpTalkTransInterface *interface,void *callBackData)
{
	Rtp * This = (Rtp *)calloc(1,sizeof(Rtp));
	This->interface = (UdpTalkTransInterface *)calloc(1,sizeof(UdpTalkTransInterface));
	SPIThreadExit = TRUE;
	RTPThreadExit = TRUE;

	call_back_data = callBackData;
	loadInterface(This->interface,interface);
	This->getFpAudio = rtpGetFpAudio;
	This->setSilence = rtpSetSilence;
	This->getSilence = rtpGetSilence;
	This->setPeerIp = rtpSetPeerIp;
	This->getVideo = rtpGetVideo;
	This->init = rtpInit;
	This->buildConnect = rtpBuildConnect;
	This->startAudio = rtpStartAudio;
	This->close = rtpClose;
	This->Destroy = rtpDestroy;

	return This;
}
