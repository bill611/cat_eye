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

static void reloadLocalTalk(void)
{
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
	// my_video->recordStart(0);
}

static void sendCmd(char *cmd,char *user_id)
{
#ifdef USE_UCPAAS
	ucsSendCmd(cmd,user_id);
#endif
}

static void playVideo(const unsigned char* frame_data, const unsigned int data_len)
{
#ifdef USE_UCPAAS
	ucsPlayVideo(frame_data,data_len);
#endif
}

static void talkConnect(void)
{
#ifdef USE_UCPAAS
	ucsConnect(local_user.token);
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
static void cbInitAudio(void)
{

}
static void cbStartRecord(void)
{

}
static void cbRecording(char *data,unsigned int size)
{

}
static void cbPlayAudio(const char *data,unsigned int size)
{

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
	protocol_talk->reload = reloadLocalTalk;
	ucsLoadInterface(&interface);
#ifdef USE_UCPAAS
	registUcpaas();
#endif
	protocol_talk->reload();
	protocol_talk->connect();
}
