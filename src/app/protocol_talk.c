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
#include "ucpaas.h"

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
static void dial(char *user_id,void (*callBack)(void *arg))
{
#ifdef USE_UCPASS
	ucsDial(user_id,callBack);
#endif
}
static void answer(void (*callBack)(void *arg))
{
#ifdef USE_UCPASS
	ucsAnswer(callBack);
#endif
}
static void hangup(void (*callBack)(void *arg))
{
#ifdef USE_UCPASS
	ucsHangup(callBack);
#endif
}
static void cbDialRet(void (*callBack)(void *arg))
{
#ifdef USE_UCPASS
	ucsCbDialRet(callBack);
#endif
}
static void cblIncomingCall(void (*callBack)(void *arg))
{
#ifdef USE_UCPASS
	ucsCbIncomingCall(callBack);
#endif
}
static void sendCmd(char *cmd,char *user_id,void (*callBack)(void *arg))
{
#ifdef USE_UCPASS
	ucsSendCmd(cmd,user_id,callBack);
#endif
}
static void receivedCmd(void (*callBack)(const char *user_id,void *arg))
{
#ifdef USE_UCPASS
	ucsCbReceivedCmd(callBack);
#endif
}
static void initAudio(void (*callBack)(void))
{
#ifdef USE_UCPASS
	ucsCbInitAudio(callBack);
#endif
}
static void playAudio(void (*callBack)(const char *data,unsigned int size))
{
#ifdef USE_UCPASS
	ucsCbPlayAudio(callBack);
#endif
}
static void startRecord(void (*callBack)(void))
{
#ifdef USE_UCPASS
	ucsCbStartRecord(callBack);
#endif
}
static void recording(void (*callBack)(char *data,unsigned int size))
{
#ifdef USE_UCPASS
	ucsCbRecording(callBack);
#endif
}
static void playVideo(const unsigned char* frame_data, const unsigned int data_len)
{
#ifdef USE_UCPASS
	ucsPlayVideo(frame_data,data_len);
#endif
}

static void talkConnect(void)
{
#ifdef USE_UCPASS
	ucsConnect(local_user.token);
#endif
}

void registTalk(void)
{
	protocol_talk = (ProtocolTalk *) calloc(1,sizeof(ProtocolTalk));
	protocol_talk->dial = dial;
	protocol_talk->answer = answer;
	protocol_talk->hangup = hangup;
	protocol_talk->connect = talkConnect;
	protocol_talk->reload = reloadLocalTalk;
	protocol_talk->cblIncomingCall = cblIncomingCall;
#ifdef USE_UCPASS
	registUcpaas();
#endif
}
