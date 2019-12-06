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
#include "cJSON.h"
#include "json_dec.h"
#include "protocol.h"
#include "my_video.h"
#include "my_mixer.h"
#include "my_gpio.h"
#include "my_audio.h"
#include "my_echo.h"
#include "ucpaas/ucpaas.h"
#include "udp_server.h"
#include "config.h"
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
	MSG_TYPE_NEED_CALL_APP,
};
enum {
	DEV_TYPE_APP = 0,
	DEV_TYPE_CATEYE,
	DEV_TYPE_OUTDOOR,
};
/* 透传协议示例，messageType为以上枚举类型
   {
	   "messageType":9,
	   "deviceType":1,
	   "deviceNumber":"机身码",
	   "fromId":"对讲Id",
	   "messageContent":"",
	   "data":{
			"electricQuantity":60, // 剩余百分60的电量
			"time":"2019-06-21 13:13"
		}

   }
*/
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
ProtocolTalk *protocol_talk;
static UserStruct local_user;
static UserStruct peer_user;
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
	if (protocol_video && protocol_talk->type == PROTOCOL_TALK_LAN) {
		protocol_video->call(protocol_video,user_id);
	} else {
		if (protocol_video)
			protocol_video->setStatusBusy(protocol_video);
#ifdef USE_UCPAAS
		ucsDial(user_id);
		memset(&peer_user,0,sizeof(UserStruct));
		strcpy(peer_user.id,user_id);
		dialCallBack = callBack;
#endif
	}
}

static void hangup(int need_transfer)
{
	printf("need:%d\n",need_transfer );
	if (protocol_talk->type == PROTOCOL_TALK_LAN) {
		myAudioStopPlay();
		if (protocol_video)
			protocol_video->hangup(protocol_video);
		if (udp_talk_trans)
			udp_talk_trans->close(udp_talk_trans);
	} else {
#ifdef USE_UCPAAS
		ucsHangup();
		if (need_transfer) {
			char send_buff[256];
			sprintf(send_buff,"{\\\"messageType\\\":%d,\\\"deviceType\\\":%d,\\\"deviceNumber\\\":\\\"%s\\\",\\\"fromId\\\":\\\"%s\\\",\\\"messageContent\\\":\\\"\\\"}",
					MSG_TYPE_NEED_CALL_APP,DEV_TYPE_CATEYE,g_config.imei,local_user.id);
			ucsSendCmd(send_buff,peer_user.id);
		}
		if (protocol_video)
			protocol_video->setStatusIdle(protocol_video);
#endif
	}
}
static void unlock(void)
{
	if (protocol_video && protocol_talk->type == PROTOCOL_TALK_LAN) {
		protocol_video->unlock(protocol_video);
	} else {
#ifdef USE_UCPAAS
		char send_buff[256];
		sprintf(send_buff,"{\\\"messageType\\\":%d,\\\"deviceType\\\":%d,\\\"deviceNumber\\\":\\\"%s\\\",\\\"fromId\\\":\\\"%s\\\",\\\"messageContent\\\":\\\"\\\"}",
				MSG_TYPE_UNLOCK,DEV_TYPE_CATEYE,g_config.imei,local_user.id);
		ucsSendCmd(send_buff,peer_user.id);
		printf("send:%s,id:%s\n",send_buff,peer_user.id );
#endif
	}
}

static void answer(void)
{
	myAudioStopPlay();
	if (protocol_talk->type == PROTOCOL_TALK_LAN) {
		if (protocol_video)
			protocol_video->answer(protocol_video);
		if (udp_talk_trans)
			udp_talk_trans->startAudio(udp_talk_trans);
	} else {
#ifdef USE_UCPAAS
		ucsAnswer();
#endif
	}
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
	if (udp_talk_trans && protocol_talk->type == PROTOCOL_TALK_LAN) {
		*size = udp_talk_trans->getVideo(udp_talk_trans,data);
	} else {
#ifdef USE_UCPAAS
		long long timeStamp = 0;
		int frameType = 0;
		ucsReceiveVideo(data, size, &timeStamp, &frameType);
#endif
	}
}
static void cbDialFail(void *arg)
{
	if (protocol_talk->type == PROTOCOL_TALK_LAN)
		return;

	if (dialCallBack)
		dialCallBack(0);
}
static void cbAnswer(void *arg)
{
	if (protocol_talk->type == PROTOCOL_TALK_LAN)
		return;

	if (my_video)
		my_video->videoAnswer(CALL_DIR_OUT,DEV_TYPE_ENTRANCEMACHINE);
}
static void cbHangup(void *arg)
{
	int type = *(int *)arg;
	if (protocol_talk->type == PROTOCOL_TALK_LAN
			|| type == 0)
		return;

	if (my_video) {
		if (type == 2)
			my_video->videoHangup(HANGUP_TYPE_PEER);
		else if (type == 1)
			my_video->videoHangup(HANGUP_TYPE_BUTTON);
	}
	if (my_mixer) {
		if (audio_fp > 0)
			my_mixer->DeInitPlay(my_mixer,&audio_fp);
	}
	dialCallBack = NULL;
	myAudioStopPlay();
}
static void cbDialRet(void *arg)
{
	if (dialCallBack)
		dialCallBack(1);
}
static void cblIncomingCall(void *arg)
{
	if (protocol_video && protocol_video->getIdleStatus(protocol_video) == 0) {
#ifdef USE_UCPAAS
		// ucsHangup();
#endif
		return;
	}
	printf("[%s]%d\n", __func__,protocol_video->getIdleStatus(protocol_video));

	protocol_talk->type = PROTOCOL_TALK_CLOUD;
	if (protocol_video)
		protocol_video->setStatusBusy(protocol_video);
	if (my_video)
		my_video->videoCallIn((char *)arg);
	strcpy(peer_user.id,(char *)arg);
}
static void cbSendCmd(void *arg)
{

}
static void cbReceivedCmd(const char *user_id,void *arg)
{
	if (protocol_talk->type == PROTOCOL_TALK_LAN)
		return;

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
	int message_type = dec->getValueInt(dec, "messageType");
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
			my_video->capture(CAP_TYPE_TALK,1,NULL,NULL);
			break;
		case MSG_TYPE_RECORD_START:
			my_video->recordStart(CAP_TYPE_TALK);
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
	if (protocol_talk->type == PROTOCOL_TALK_LAN)
		return;

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
	if (protocol_talk->type == PROTOCOL_TALK_LAN)
		return;

    char audio_buff[1024] = {0};
	if (my_mixer ) {
		int real_size = my_mixer->Read(my_mixer,audio_buff,size,2);
		if (mic_open) {
            memcpy(data,audio_buff,real_size);
		}
        if (my_video)
            my_video->recordWriteCallback(audio_buff,real_size);
	}
}
static void cbPlayAudio(const char *data,unsigned int size)
{
	if (protocol_talk->type == PROTOCOL_TALK_LAN)
		return;

	if (my_mixer) {
		my_mixer->Write(my_mixer,audio_fp,data,size);
	}
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
			protocol_talk->type = PROTOCOL_TALK_LAN;
			break;
		case VIDEOTRANS_UI_CALLIP:			// 呼出
			break;
		case VIDEOTRANS_UI_RING:				// 呼入响铃
			{
				protocol_talk->type = PROTOCOL_TALK_LAN;
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
				my_video->videoHangup(1);
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
		return my_mixer->Read(my_mixer,data,size,1 );
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
	if (my_mixer) {
		my_mixer->SetVolumeEx(my_mixer,g_config.talk_volume);
		my_mixer->InitPlayAndRec(my_mixer,&audio_fp,8000,1);
	}

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
	if (protocol_video)
		protocol_video->cmdHandle(protocol_video,ip,port,data,size);
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
	protocol_talk->unlock = unlock;
	protocol_talk->type = PROTOCOL_TALK_CLOUD;
#ifdef USE_UCPAAS
	registUcpaas(&interface);
	protocol_talk->reload();
	protocol_talk->connect();
#endif

	protocol_talk->udpCmd = udpCmd;
	protocol_video = videoTransCreate(&video_interface,
			        7800,0,3,VIDEOTRANS_PROTOCOL_3000,"","123","0101");
	protocol_video->enable(protocol_video);
	udp_talk_trans = createRtp(&rtp_interface,protocol_video);
	rkEchoInit();
}
