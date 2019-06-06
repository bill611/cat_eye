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
	ucsDial(user_id,callBack);
}
static void answer(void (*callBack)(void *arg))
{
	ucsAnswer(callBack);
}
static void hangup(void (*callBack)(void *arg))
{
	ucsHangup(callBack);
}
static void cbDialRet(void (*callBack)(void *arg))
{
	ucsCbDialRet(callBack);
}
static void cblIncomingCall(void (*callBack)(void *arg))
{
	ucsCbIncomingCall(callBack);
}
static void sendCmd(char *cmd,char *user_id,void (*callBack)(void *arg))
{
	ucsSendCmd(cmd,user_id,callBack);
}
static void receivedCmd(void (*callBack)(const char *user_id,void *arg))
{
	ucsCbReceivedCmd(callBack);
}
static void initAudio(void (*callBack)(void))
{
	ucsCbInitAudio(callBack);
}
static void playAudio(void (*callBack)(const char *data,unsigned int size))
{
	ucsCbPlayAudio(callBack);
}
static void startRecord(void (*callBack)(void))
{
	ucsCbStartRecord(callBack);
}
static void recording(void (*callBack)(char *data,unsigned int size))
{
	ucsCbRecording(callBack);
}
static void playVideo(const unsigned char* frame_data, const unsigned int data_len)
{
	ucsPlayVideo(frame_data,data_len);
}

static void talkConnect(void)
{
	ucsConnect(local_user.token);
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
#ifndef X86
	registUcpaas();
#endif
}
