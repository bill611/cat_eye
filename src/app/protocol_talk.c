/*
 * =============================================================================
 *
 *       Filename:  protocol_talk.c
 *
 *    Description:  对讲协议
 *
 *        Version:  1.0
 *        Created:  2019-06-05 10:29:06
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
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "sql_handle.h"
#include "json_dec.h"
#include "protocol.h"
#include "my_video.h"
#include "my_mixer.h"
#include "my_gpio.h"
#include "my_audio.h"
#include "ucpaas/ucpaas.h"
#include "udp_server.h"
#include "udp_talk/udp_talk_protocol.h"
#include "udp_talk/udp_talk_transport.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
enum {
	MSG_TYPE_CALL = 1,
	MSG_TYPE_UNLOCK,
	MSG_TYPE_SLEEP,
	MSG_TYPE_MIC_CLOSE,
	MSG_TYPE_MIC_OPEN,
	MSG_TYPE_CAPTURE,
	MSG_TYPE_RECORD_START,
	MSG_TYPE_RECORD_STOP,
};
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
ProtocolTalk *protocol_talk;
static UserStruct local_user;
static int has_connect = 0; // 判断是否有连接过，如果有，则重连调用disconnect
static int audio_fp = -1;
static int mic_open = 0; // mic状态 0关闭 1开启
static void (*dialCallBack)(int result);

static void reloadLocalTalk(void)
{
    memset(&local_user,0,sizeof(UserStruct));
	sqlGetUserInfoUseType(USER_TYPE_CATEYE,local_user.id,local_user.token,local_user.nick_name,&local_user.scope);
	printf("[%s]id:%s,token:%s\n",__func__,local_user.id,local_user.token );
}
static void dial(char *user_id,void (*callBack)(int result))
{
#ifdef USE_UCPAAS
	ucsDial(user_id);
#endif
	dialCallBack = callBack;
#ifdef USE_UDPTALK
	if (protocol_video)
		protocol_video->call(protocol_video,user_id);
#endif
}

static void hangup(void)
{
#ifdef USE_UCPAAS
	ucsHangup();
#endif
	myAudioStopPlay();
#ifdef USE_UDPTALK
	if (protocol_video)
		protocol_video->hangup(protocol_video);
	if (udp_talk_trans)
		udp_talk_trans->close(udp_talk_trans);
#endif
}
static void unlock(void)
{
#ifdef USE_UDPTALK
	if (protocol_video)
		protocol_video->unlock(protocol_video);
#endif
}

static void answer(void)
{
#ifdef USE_UCPAAS
	ucsAnswer();
#endif
	myAudioStopPlay();
#ifdef USE_UDPTALK
	if (protocol_video)
		protocol_video->answer(protocol_video);
	if (udp_talk_trans)
		udp_talk_trans->startAudio(udp_talk_trans);
#endif
}

static void sendCmd(char *cmd,char *user_id)
{
#ifdef USE_UCPAAS
	ucsSendCmd(cmd,user_id);
#endif
}

static void talkConnect(void)
{
#ifdef USE_UCPAAS
	if (ucsConnect(local_user.token))
        has_connect = 1;
#endif
}
static void talkReconnect(void)
{
#ifdef USE_UCPAAS
    if (has_connect)
        ucsDisconnect();
	ucsConnect(local_user.token);
#endif
}

static void sendVideo(void *data,int size)
{
#ifdef USE_UCPAAS
	ucsSendVideo(data,size);
#endif
}
static void receiveVideo(void *data,int *size)
{
#ifdef USE_UCPAAS
	long long timeStamp = 0;
	int frameType = 0;
	ucsReceiveVideo(data, size, &timeStamp, &frameType);
#endif
#ifdef USE_UDPTALK
	if (udp_talk_trans)
		*size = udp_talk_trans->getVideo(udp_talk_trans,data);
#endif
}
static void cbDialFail(void *arg)
{
	if (dialCallBack)
		dialCallBack(0);
}
static void cbAnswer(void *arg)
{
	if (my_video)
		my_video->videoAnswer(1,DEV_TYPE_ENTRANCEMACHINE);
}
static void cbHangup(void *arg)
{
	if (my_video)
		my_video->videoHangup();
	if (my_mixer) {
		if (audio_fp > 0)
			my_mixer->DeInitPlay(my_mixer,&audio_fp);
	}
	dialCallBack = NULL;
}
static void cbDialRet(void *arg)
{
	if (dialCallBack)
		dialCallBack(1);
}
static void cblIncomingCall(void *arg)
{
	if (my_video)
		my_video->videoCallIn((char *)arg);
}
static void cbSendCmd(void *arg)
{

}
static void cbReceivedCmd(const char *user_id,void *arg)
{
	printf("%s\n", (char *)arg);
	char *data = (char *)arg;
	char *j_data = (char *)calloc(1,strlen(data));
	char *p = j_data;
	while (*data != '\0') {
		if (*data != '\\') {
			*p = *data;
			p++;
		}
		data++;
	}
	CjsonDec *dec = cjsonDecCreate(j_data);
	if (!dec) {
		printf("json dec create fail!\n");
		if (j_data)
			free(j_data);
		return ;
	}
	dec->print(dec);
	int message_type = dec->getValueInt(dec, "deviceType");
	switch (message_type)
	{
		case MSG_TYPE_UNLOCK :
			break;
		case MSG_TYPE_MIC_OPEN :
			mic_open = 1;
			break;
		case MSG_TYPE_MIC_CLOSE :
			mic_open = 0;
			break;
		case MSG_TYPE_CAPTURE :
			my_video->capture(0);
			break;
		case MSG_TYPE_RECORD_START:
			my_video->recordStart(0);
			break;
		case MSG_TYPE_RECORD_STOP:
			my_video->recordStop();
			break;
		default:
			break;
	}
	if (j_data)
		free(j_data);
	if (dec)
		dec->destroy(dec);
}
static void cbInitAudio(unsigned int rate,unsigned int bytes_per_sample,unsigned int channle)
{
	gpio->SetValue(gpio,ENUM_GPIO_MICKEY,IO_ACTIVE);
	mic_open = 1;
	if (my_mixer)
		my_mixer->InitPlayAndRec(my_mixer,&audio_fp,rate,channle);
}
static void cbStartRecord(unsigned int rate,unsigned int bytes_per_sample,unsigned int channle)
{

}
static void cbRecording(char *data,unsigned int size)
{
	// printf("mic :%d\n", mic_open);
	if (my_mixer && mic_open)
		my_mixer->Read(my_mixer,data,size);
}
static void cbPlayAudio(const char *data,unsigned int size)
{
	if (my_mixer)
		my_mixer->Write(my_mixer,audio_fp,data,size);
}

static Callbacks interface = {
	.dialFail = cbDialFail,
	.answer = cbAnswer,
	.hangup = cbHangup,
	.dialRet = cbDialRet,
	.incomingCall = cblIncomingCall,
	.sendCmd = cbSendCmd,
	.receivedCmd = cbReceivedCmd,
	.initAudio = cbInitAudio,
	.startRecord = cbStartRecord,
	.recording = cbRecording,
	.playAudio = cbPlayAudio,
};

static void videoCallbackOvertime(int ret,void *CallBackData)
{
	if(ret != MSG_SENDTIMEOUT) {
		return ;
	}
	VideoTrans * This = (VideoTrans *)CallBackData;
	This->callBackOverTime(This);
}
static void videoUdpSend(VideoTrans *This,
		char *ip,int port,void *data,int size,int enable_call_back)
{
	if (enable_call_back)
		udp_server->AddTask(udp_server,
				ip,port,data,
				size,3,videoCallbackOvertime,This);
	else
		udp_server->AddTask(udp_server,
				ip,port,data,
				size,3,NULL,NULL);
}

static void videoSendMessageStatus(VideoTrans *This,VideoUiStatus status)
{
	switch(status)
	{
		case VIDEOTRANS_UI_NONE:				// 不做处理
			break;
		case VIDEOTRANS_UI_SHAKEHANDS:		// 3000握手命令
			break;
		case VIDEOTRANS_UI_CALLIP:			// 呼出
			break;
		case VIDEOTRANS_UI_RING:				// 呼入响铃
			{
				if (my_video)
					my_video->videoCallIn("门口机");
				if (udp_talk_trans) {
					udp_talk_trans->init(udp_talk_trans,This->getPeerIP(This),8800);
					udp_talk_trans->buildConnect(udp_talk_trans);
				}
				myAudioPlayRing();
			}
			break;
		case VIDEOTRANS_UI_LEAVE_WORD:		// 留言
			break;
		case VIDEOTRANS_UI_RETCALL:			// 呼出到管理中心，室内机收到回应
		case VIDEOTRANS_UI_RETCALL_MONITOR:	// 呼出到门口机，户门口机收到回应,显示开锁按钮
			if (udp_talk_trans) {
				udp_talk_trans->init(udp_talk_trans,This->getPeerIP(This),8800);
				udp_talk_trans->buildConnect(udp_talk_trans);
			}
			break;
		case VIDEOTRANS_UI_FAILCOMM:			// 通信异常
			break;
		case VIDEOTRANS_UI_FAILSHAKEHANDS:	// 握手异常
			break;
		case VIDEOTRANS_UI_FAILBUSY:			// 对方忙
			break;
		case VIDEOTRANS_UI_FAILABORT:		// 突然中断
			break;
		case VIDEOTRANS_UI_ANSWER:			// 本机接听
			break;
		case VIDEOTRANS_UI_ANSWER_EX: 		// 分机接听
			break;
		case VIDEOTRANS_UI_OVER:				// 挂机
			if (my_video)
				my_video->videoHangup();
			break;
		default:
			break;
	}	
}

static VideoInterface video_interface = {
	.udpSend = videoUdpSend,
	.sendMessageStatus = videoSendMessageStatus,
};

static int udpSendAudio(Rtp *This,void *data,int size)
{
	if (my_mixer)
		return my_mixer->Read(my_mixer,data,size / 4);
	else
		return 0;
}
static void udpReceiveAudio(Rtp *This,void *data,int size)
{
	if (my_mixer)
		my_mixer->Write(my_mixer,audio_fp,data,size);
}
static void udpStart(Rtp *This)
{
	gpio->SetValue(gpio,ENUM_GPIO_MICKEY,IO_ACTIVE);
	printf("[%s]\n", __func__);
	if (my_mixer)
		my_mixer->InitPlayAndRec(my_mixer,&audio_fp,8000,1);

}
static void udpReceiveEnd(Rtp *This)
{
	printf("[%s]\n", __func__);
	if (my_mixer) {
		if (audio_fp > 0)
			my_mixer->DeInitPlay(my_mixer,&audio_fp);
	}
}
static void udpCmd(char *ip,int port, char *data,int size)
{
#ifdef USE_UDPTALK
	if (protocol_video)
		protocol_video->cmdHandle(protocol_video,ip,port,data,size);
#endif
}
static UdpTalkTransInterface rtp_interface = {
	.receiveAudio = udpReceiveAudio,
	.receiveEnd = udpReceiveEnd,
	.sendAudio = udpSendAudio,
	.start = udpStart,
};

void registTalk(void)
{
	protocol_talk = (ProtocolTalk *) calloc(1,sizeof(ProtocolTalk));
	protocol_talk->dial = dial;
	protocol_talk->answer = answer;
	protocol_talk->hangup = hangup;
	protocol_talk->connect = talkConnect;
	protocol_talk->reconnect = talkReconnect;
	protocol_talk->reload = reloadLocalTalk;
	protocol_talk->sendVideo = sendVideo;
	protocol_talk->receiveVideo = receiveVideo;
	protocol_talk->udpCmd = udpCmd;
	protocol_talk->unlock = unlock;
#ifdef USE_UCPAAS
	protocol_talk->type = PROTOCOL_TALK_OTHER;
	registUcpaas(&interface);
	protocol_talk->reload();
	protocol_talk->connect();
#endif
#ifdef USE_UDPTALK
	protocol_talk->type = PROTOCOL_TALK_3000;
	protocol_video = videoTransCreate(&video_interface,
			        7800,0,3,VIDEOTRANS_PROTOCOL_3000,"","123","0101");
	protocol_video->enable(protocol_video);
	udp_talk_trans = createRtp(&rtp_interface,protocol_video);
#endif
}
