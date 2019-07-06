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
#include "ucpaas/ucpaas.h"

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
}

static void hangup(void)
{
#ifdef USE_UCPAAS
	ucsHangup();
#endif
}

static void answer(void)
{
#ifdef USE_UCPAAS
	ucsAnswer();
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
		my_mixer->InitPlayAndRec(my_mixer,&audio_fp,rate,2);
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
#ifdef USE_UCPAAS
	registUcpaas(&interface);
#endif
	protocol_talk->reload();
	protocol_talk->connect();
}
