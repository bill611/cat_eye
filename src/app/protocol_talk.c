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
#include "protocol.h"
#include "my_video.h"
#include "my_mixer.h"
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
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
ProtocolTalk *protocol_talk;
static UserStruct local_user;
static int has_connect = 0; // 判断是否有连接过，如果有，则重连调用disconnect
static int audio_fp = 0;

static void reloadLocalTalk(void)
{
    memset(&local_user,0,sizeof(UserStruct));
	sqlGetUserInfo(USER_TYPE_CATEYE,local_user.id,local_user.token,local_user.nick_name,&local_user.scope);
	printf("[%s]id:%s,token:%s\n",__func__,local_user.id,local_user.token );
}
static void dial(char *user_id)
{
#ifdef USE_UCPAAS
	ucsDial(user_id);
#endif
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
static void cbDialFail(void *arg)
{

}
static void cbAnswer(void *arg)
{

}
static void cbHangup(void *arg)
{
	if (protocol_talk->uiHangup)
		protocol_talk->uiHangup();
	if (my_video)
		my_video->transVideoStop();
	if (my_mixer) {
		if (audio_fp > 0)
			my_mixer->DeInitPlay(my_mixer,&audio_fp);	
	}
}
static void cbDialRet(void *arg)
{

}
static void cblIncomingCall(void *arg)
{
	if (protocol_talk->uiIncomingCall)
		protocol_talk->uiIncomingCall(arg);
	if (my_video)
		my_video->transVideoStart();
	protocol_talk->answer();
}
static void cbSendCmd(void *arg)
{

}
static void cbReceivedCmd(const char *user_id,void *arg)
{

}
static void cbInitAudio(unsigned int rate,unsigned int bytes_per_sample,unsigned int channle)
{
	if (my_mixer)
		my_mixer->InitPlayAndRec(my_mixer,&audio_fp,rate,2);
}
static void cbStartRecord(unsigned int rate,unsigned int bytes_per_sample,unsigned int channle)
{

}
static void cbRecording(char *data,unsigned int size)
{

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
	ucsLoadInterface(&interface);
#ifdef USE_UCPAAS
	registUcpaas();
#endif
	protocol_talk->reload();
	protocol_talk->connect();
}
