/*
 * =============================================================================
 *
 *       Filename:  my_video.c
 *
 *    Description:  视频接口
 *
 *        Version:  1.0
 *        Created:  2019-06-19 10:19:50
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "debug.h"
#include "h264_enc_dec/mpi_enc_api.h"
#include "jpeg_enc_dec.h"
#include "video_server.h"
#include "my_face.h"
#include "state_machine.h"
#include "ucpaas/ucpaas.h"
#include "sql_handle.h"
#include "protocol.h"
#include "thread_helper.h"
#include "timer.h"
#include "my_video.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define TIME_TALKING 180
#define TIME_CALLING 30
enum {
	EV_FACE_ON,			// 打开人脸识别功能
	EV_FACE_OFF,		// 关闭人脸识别功能
	EV_FACE_OFF_FINISH,	// 关闭人脸识别功能结束
	EV_FACE_REGIST,		// 注册人脸
	EV_TALK_CALLOUT,	// 对讲呼出
	EV_TALK_CALLOUTALL,	// 遍历对讲呼出
	EV_TALK_CALLOUTOK,	// 对讲呼出成功
	EV_TALK_CALLIN,	    // 对讲呼入
	EV_TALK_ANSWER,	    // 对讲接听
	EV_TALK_HANGUP,	    // 对讲挂机
	EV_TALK_HANGUPALL,	// 遍历对讲挂机
	EV_CAPTURE,	    	// 截图
	EV_RECORD_START,	// 录像开始
	EV_RECORD_STOP,	    // 录像结束
	EV_RECORD_STOP_FINISHED,// 录像结束后执行动作
};

enum {
	ST_IDLE,		// 空闲状态
	ST_FACE,		// 人脸识别开启状态
	ST_TALK_CALLOUT,// 对讲呼出状态
	ST_TALK_CALLOUTALL,// 遍历对讲呼出状态
	ST_TALK_CALLIN,	// 对讲呼入状态
	ST_TALK_TALKING,// 对讲中状态
	ST_RECORDING, 	// 录像状态
	ST_CAPTURE, 	// 抓拍状态
};

enum {
	DO_FAIL, 		// 信息发送失败
	DO_NOTHING, 	// 不做任何操作
	DO_FACE_ON, 	// 人脸识别开启
	DO_FACE_OFF, 	// 人脸识别关闭
    DO_FACE_REGIST, // 注册人脸
    DO_TALK_CALLOUT, 	// 对讲呼出
    DO_TALK_CALLOUTALL, // 遍历对讲呼出
    DO_TALK_CALLIN, 	// 对讲呼入
    DO_TALK_ANSWER, 	// 对讲接听
    DO_TALK_HANGUP, 	// 对讲挂机
    DO_TALK_HANGUPALL, 	// 遍历对讲挂机
    DO_CAPTURE, 		// 抓拍
    DO_RECORD_START, 	// 录像开始
    DO_RECORD_STOP, 	// 录像结束
};

typedef struct _StmData {
	int type;
	int call_dir; // 操作方向 0 本机，1对方
	char nick_name[128];
	char usr_id[128];
}StmData;
typedef struct _TalkPeerDev {
	char peer_nick_name[128];
	int call_out_result;
	int call_time;
	int type;
}TalkPeerDev;

typedef struct _StmDo {
	int action;
	int (*proc)(void *data);
}StmDo;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
MyVideo *my_video;
static char *st_debug_ev[] = {
	"EV_FACE_ON",			// 打开人脸识别功能
	"EV_FACE_OFF",          // 关闭人脸识别功能
	"EV_FACE_OFF_FINISH",   // 关闭人脸识别功能结束
	"EV_FACE_REGIST",       // 注册人脸
	"EV_TALK_CALLOUT",      // 对讲呼出
	"EV_TALK_CALLOUTALL",   // 遍历对讲呼出
	"EV_TALK_CALLIN",       // 对讲呼入
	"EV_TALK_ANSWER",       // 对讲接听
	"EV_TALK_HANGUP",       // 对讲挂机
	"EV_TALK_HANGUPALL",	// 遍历对讲挂机
	"EV_CAPTURE",           // 遍历对讲挂机
	"EV_RECORD_START",      // 截图
	"EV_RECORD_STOP",       // 录像开始
	"EV_RECORD_STOP_FINISHED",// 录像结束后执行动作
};
static char *st_debug_st[] = {
	"ST_IDLE",
	"ST_FACE",
	"ST_TALK_CALLOUT",
	"ST_TALK_CALLOUTALL",
	"ST_TALK_CALLIN",
	"ST_TALK_TALKING",
	"ST_RECORDING",
	"ST_CAPTURE",
};
static char *st_debug_do[] = {
	"DO_FAIL",
	"DO_NOTHING",
	"DO_FACE_ON",
	"DO_FACE_OFF",
    "DO_FACE_REGIST",
    "DO_TALK_CALLOUT",
    "DO_TALK_CALLOUTALL",
    "DO_TALK_CALLIN",
    "DO_TALK_ANSWER",
    "DO_TALK_HANGUP",
    "DO_TALK_HANGUPALL", 	// 遍历对讲挂机
    "DO_CAPTURE",
    "DO_RECORD_START",
    "DO_RECORD_STOP",
};

static StateTableDebug st_debug = {
	.ev = st_debug_ev,
	.st = st_debug_st,
	.todo = st_debug_do,
};
static StmData *st_data = NULL;
static StMachine* stm;

static TalkPeerDev talk_peer_dev;

static StateTable state_table[] =
{
	{EV_FACE_ON,		ST_IDLE,	ST_FACE,	DO_FACE_ON},
	{EV_FACE_OFF,		ST_FACE,	ST_IDLE,	DO_FACE_OFF},
	{EV_FACE_REGIST,	ST_FACE,	ST_FACE,	DO_FACE_REGIST},

	{EV_FACE_OFF_FINISH,ST_TALK_CALLOUT,	ST_TALK_CALLOUT,	DO_TALK_CALLOUT},
	{EV_FACE_OFF_FINISH,ST_TALK_CALLOUTALL,	ST_TALK_CALLOUTALL,	DO_TALK_CALLOUTALL},
	{EV_FACE_OFF_FINISH,ST_TALK_CALLIN,		ST_TALK_CALLIN,		DO_TALK_CALLIN},
	{EV_FACE_OFF_FINISH,ST_RECORDING,		ST_RECORDING,		DO_RECORD_START},

	{EV_TALK_CALLOUT,	ST_IDLE,			ST_TALK_CALLOUT,	DO_TALK_CALLOUT},
	{EV_TALK_CALLOUT,	ST_FACE,			ST_TALK_CALLOUT,	DO_FACE_OFF},
	{EV_TALK_CALLOUT,	ST_TALK_CALLOUTALL,	ST_TALK_CALLOUTALL,	DO_TALK_CALLOUT},

	{EV_TALK_CALLOUTALL,ST_IDLE,			ST_TALK_CALLOUTALL,	DO_TALK_CALLOUTALL},
	{EV_TALK_CALLOUTALL,ST_FACE,			ST_TALK_CALLOUTALL,	DO_FACE_OFF},

	{EV_TALK_CALLOUTOK,	ST_TALK_CALLOUTALL,	ST_TALK_CALLOUT,	DO_NOTHING},

	{EV_TALK_CALLIN,	ST_IDLE,			ST_TALK_CALLIN,		DO_TALK_CALLIN},
	{EV_TALK_CALLIN,	ST_FACE,			ST_TALK_CALLIN,		DO_FACE_OFF},

	{EV_TALK_ANSWER,	ST_TALK_CALLIN,		ST_TALK_TALKING,	DO_TALK_ANSWER},
	{EV_TALK_ANSWER,	ST_TALK_CALLOUT,	ST_TALK_TALKING,	DO_TALK_ANSWER},
	{EV_TALK_ANSWER,	ST_TALK_CALLOUTALL,	ST_TALK_TALKING,	DO_TALK_ANSWER},

	{EV_TALK_HANGUP,	ST_TALK_CALLOUT,	ST_IDLE,			DO_TALK_HANGUP},
	{EV_TALK_HANGUP,	ST_TALK_CALLIN,		ST_IDLE,			DO_TALK_HANGUP},
	{EV_TALK_HANGUP,	ST_TALK_TALKING,	ST_IDLE,			DO_TALK_HANGUP},
	{EV_TALK_HANGUP,	ST_TALK_CALLOUTALL,	ST_TALK_CALLOUTALL,	DO_TALK_HANGUPALL},

	{EV_TALK_HANGUPALL,	ST_TALK_CALLOUTALL,	ST_IDLE,			DO_TALK_HANGUP},

	{EV_RECORD_START,	ST_IDLE,			ST_RECORDING,		DO_RECORD_START},
	{EV_RECORD_START,	ST_FACE,			ST_RECORDING,		DO_FACE_OFF},
	{EV_RECORD_START,	ST_TALK_CALLIN,		ST_TALK_CALLIN,		DO_RECORD_START},
	{EV_RECORD_START,	ST_TALK_CALLOUT,	ST_TALK_CALLOUT,	DO_RECORD_START},
	{EV_RECORD_START,	ST_TALK_TALKING,	ST_TALK_TALKING,	DO_RECORD_START},

	{EV_RECORD_STOP,	ST_RECORDING,		ST_IDLE,			DO_RECORD_STOP},
	{EV_RECORD_STOP,	ST_TALK_CALLIN,		ST_TALK_CALLIN,		DO_RECORD_STOP},
	{EV_RECORD_STOP,	ST_TALK_CALLOUT,	ST_TALK_CALLOUT,	DO_RECORD_STOP},
	{EV_RECORD_STOP,	ST_TALK_TALKING,	ST_TALK_TALKING,	DO_RECORD_STOP},

	{EV_RECORD_STOP_FINISHED,	ST_IDLE,	ST_FACE,			DO_FACE_ON},

};

static int stmDoFail(void *data)
{
	int msg = *(int *)data;
    switch (msg)
    {
        case DO_FACE_ON:
            break;
        case DO_FACE_OFF:
            break;
        default:
            break;
    }
	printf("%s()%s\n",__func__,st_debug_ev[msg]);
}
static int stmDoNothing(void *data)
{
	
}

static int stmDoFaceOn(void *data)
{
#ifdef USE_VIDEO
    rkVideoFaceOnOff(1);
#endif
}

static int stmDoFaceOff(void *data)
{
#ifdef USE_VIDEO
    rkVideoFaceOnOff(0);
#endif
	st_data = (StmData *)stm->initPara(stm,
			        sizeof(StmData));
	if(data)
		memcpy(st_data,data,sizeof(StmData));
	stm->msgPost(stm,EV_FACE_OFF_FINISH,st_data);
}

static int stmDoFaceRegist(void *data)
{
	if (my_face)
		return my_face->regist((MyFaceRegistData *)data);
	else
		return -1;
}

static void sendVideoCallbackFunc(void *data,int size)
{
    protocol_talk->sendVideo(data,size);
}
static void dialCallBack(int result)
{
	if (result) {
		talk_peer_dev.call_out_result = 1;
		stm->msgPost(stm,EV_TALK_CALLOUTOK,NULL);
		if (		talk_peer_dev.type != DEV_TYPE_ENTRANCEMACHINE
				|| 	talk_peer_dev.type != DEV_TYPE_HOUSEENTRANCEMACHINE) {
#ifdef USE_VIDEO
			rkH264EncOn(320,240,sendVideoCallbackFunc);
#endif
		}
	} else {
		talk_peer_dev.call_out_result = 0;
		stm->msgPost(stm,EV_TALK_HANGUP,NULL);
	}
}
static int stmDoTalkCallout(void *data)
{
	StmData *data_temp = (StmData *)data;
	int ret = sqlGetUserInfoUseUserId(data_temp->usr_id,data_temp->nick_name,&data_temp->type);
	if (ret == 0) {
		printf("can't find usr_id:%s\n", data_temp->usr_id);
		stm->msgPost(stm,EV_TALK_HANGUP,NULL);
		return -1;
	}
	char ui_title[128] = {0};
	sprintf(ui_title,"正在呼叫 %s",data_temp->nick_name);
	if (protocol_talk->uiShowFormVideo)
		protocol_talk->uiShowFormVideo(data_temp->type,ui_title);
	talk_peer_dev.call_out_result = 0;
	talk_peer_dev.type = data_temp->type;
	protocol_talk->dial(data_temp->usr_id,dialCallBack);
}

static void *threadCallOutAll(void *arg)
{
	char user_id[128] = {0};
	int index = 0;
	int user_num = sqlGetUserInfoUseScopeStart(DEV_TYPE_HOUSEHOLDAPP);
	sqlGetUserInfoEnd();
	while (user_num) {
		sqlGetUserInfosUseScopeIndex(user_id,talk_peer_dev.peer_nick_name,DEV_TYPE_HOUSEHOLDAPP,index++);
		my_video->videoCallOut(user_id);
		printf("[%s]id:%s,name:%s\n", __func__,user_id,talk_peer_dev.peer_nick_name);
		sleep(2);
		if (talk_peer_dev.call_out_result == 1)
			break;
		user_num--;
	}
	if (user_num == 0)
		stm->msgPost(stm,EV_TALK_HANGUPALL,NULL);

	return NULL;
}
static int stmDoTalkCalloutAll(void *data)
{
	createThread(threadCallOutAll,NULL);
}

static int stmDoTalkCallin(void *data)
{
	char ui_title[128] = {0};
	StmData *data_temp = (StmData *)data;
	int ret = sqlGetUserInfoUseUserId(data_temp->usr_id,data_temp->nick_name,&data_temp->type);
	if (ret == 0) {
		strcpy(data_temp->nick_name,data_temp->usr_id);
		data_temp->type = DEV_TYPE_UNDEFINED;
		printf("can't find usr_id:%s\n", data_temp->usr_id);
	}
	strcpy(talk_peer_dev.peer_nick_name,data_temp->nick_name);

	sprintf(ui_title,"%s 正在呼叫",talk_peer_dev.peer_nick_name);
	if (protocol_talk->uiShowFormVideo)
		protocol_talk->uiShowFormVideo(data_temp->type,ui_title);

	if (data_temp->type == DEV_TYPE_HOUSEHOLDAPP) {
		my_video->videoAnswer(0,data_temp->type);
	} else {
		
	}
	return 0;
}
static int stmDoTalkAnswer(void *data)
{
	char ui_title[128] = {0};
	if (		talk_peer_dev.type != DEV_TYPE_ENTRANCEMACHINE
			|| 	talk_peer_dev.type != DEV_TYPE_HOUSEENTRANCEMACHINE) {
#ifdef USE_VIDEO
		rkH264EncOn(320,240,sendVideoCallbackFunc);
#endif
	}
	memset(ui_title,0,sizeof(ui_title));
	StmData *data_temp = (StmData *)data;
	if (data_temp->type == DEV_TYPE_HOUSEHOLDAPP) {
		sprintf(ui_title,"%s 正在监视猫眼",talk_peer_dev.peer_nick_name);
	} else {
		sprintf(ui_title,"%s 正在与猫眼通话",talk_peer_dev.peer_nick_name);
	}
	protocol_talk->answer();
	if (protocol_talk->uiAnswer)
		protocol_talk->uiAnswer(ui_title);
	talk_peer_dev.call_time = TIME_TALKING;
}

static int stmDoTalkHangupAll(void *data)
{
#ifdef USE_VIDEO
	rkH264EncOff();
#endif
	protocol_talk->uiHangup();
	talk_peer_dev.call_time = 0;
	memset(&talk_peer_dev,0,sizeof(talk_peer_dev));
}

static int stmDoTalkHangup(void *data)
{
	stmDoTalkHangupAll(data);
	stm->msgPost(stm,EV_FACE_ON,NULL);
}

static int stmDoCapture(void *data)
{
	// rkVideoStopCapture();
}
FILE *fp;
static void recordEncCallbackFunc(void *data,int size)
{
	fwrite(data,1,size,fp);
}
static int stmDoRecordStart(void *data)
{
#ifdef USE_VIDEO
	fp = fopen("test.h264","wb");
	rkH264EncOn(320,240,recordEncCallbackFunc);
#endif
}
static int stmDoRecordStop(void *data)
{
#ifdef USE_VIDEO
	rkH264EncOff();
	fflush(fp);
	fclose(fp);
#endif
	stm->msgPost(stm,EV_RECORD_STOP_FINISHED,NULL);
}

static StmDo stm_do[] =
{
	{DO_FAIL,			stmDoFail},
	{DO_NOTHING,		stmDoNothing},
	{DO_FACE_ON,    	stmDoFaceOn},
	{DO_FACE_OFF,   	stmDoFaceOff},
	{DO_FACE_REGIST,	stmDoFaceRegist},
	{DO_TALK_CALLOUT,	stmDoTalkCallout},
	{DO_TALK_CALLOUTALL,stmDoTalkCalloutAll},
	{DO_TALK_CALLIN,	stmDoTalkCallin},
	{DO_TALK_ANSWER,	stmDoTalkAnswer},
	{DO_TALK_HANGUP,	stmDoTalkHangup},
	{DO_TALK_HANGUPALL,	stmDoTalkHangupAll},
	{DO_CAPTURE,		stmDoCapture},
	{DO_RECORD_START,	stmDoRecordStart},
	{DO_RECORD_STOP,	stmDoRecordStop},
};

static int stmHandle(StMachine *This,int result,void *data)
{
	if (result) {
		return stm_do[This->getCurRun(This)].proc(data);
	} else {
		return stm_do[DO_FAIL].proc(data);
	}
}

static void init(void)
{
	jpegIncDecInit();
	myFaceInit();
#ifdef USE_VIDEO
	rkVideoInit();
#endif
}

static void faceStart(void)
{
    stm->msgPost(stm,EV_FACE_ON,NULL);
}
static void faceStop(void)
{
    stm->msgPost(stm,EV_FACE_OFF,NULL);
}

static void capture(int count)
{
	stm->msgPost(stm,EV_CAPTURE,NULL);
}

static void recordStart(int count)
{
    stm->msgPost(stm,EV_RECORD_START,NULL);
}

static void recordStop(void)
{
    stm->msgPost(stm,EV_RECORD_STOP,NULL);
}

static void videoCallOut(char *user_id)
{
	st_data = (StmData *)stm->initPara(stm,
			        sizeof(StmData));
	strcpy(st_data->usr_id,user_id);
	stm->msgPost(stm,EV_TALK_CALLOUT,st_data);
	talk_peer_dev.call_time = TIME_CALLING;
}

static void videoCallOutAll(void)
{
	stm->msgPost(stm,EV_TALK_CALLOUTALL,NULL);
}

static void videoCallIn(char *user_id)
{
	st_data = (StmData *)stm->initPara(stm,
			        sizeof(StmData));
	strcpy(st_data->usr_id,user_id);
	stm->msgPost(stm,EV_TALK_CALLIN,st_data);
}
static void videoHangup(void)
{
	stm->msgPost(stm,EV_TALK_HANGUP,NULL);
}
static void videoAnswer(int dir,int dev_type)
{
	st_data = (StmData *)stm->initPara(stm,
			        sizeof(StmData));
	st_data->call_dir = dir;
	st_data->type = dev_type;
	stm->msgPost(stm,EV_TALK_ANSWER,st_data);
}
static void showVideo(void)
{
#ifdef USE_VIDEO
	rkVideoDisplayOnOff(1);
#endif
	faceStart();
}
static void hideVideo(void)
{
#ifdef USE_VIDEO
	rkVideoDisplayOnOff(0);
#endif
}

static int faceRegist( unsigned char *image_buff,int w,int h,char *id,char *nick_name,char *url)
{
    MyFaceRegistData face_data;
    face_data.image_buff = image_buff;
    face_data.w = w;
    face_data.h = h;
    face_data.id = id;
    face_data.nick_name = nick_name;
    face_data.url = url;
    return stm->msgPostSync(stm,EV_FACE_REGIST,&face_data);
}
static void faceDelete(char *id)
{
    if (my_face)
        my_face->deleteOne(id);
}

static void* videoTimerThread(void *arg)
{
	while (1) {
		if (talk_peer_dev.call_time) {
			printf("call time:%d\n", talk_peer_dev.call_time);
			if (--talk_peer_dev.call_time == 0) {
				stm->msgPost(stm,EV_TALK_HANGUP,NULL);
			}
		}
		sleep(1);
	}
	return NULL;
}
void myVideoInit(void)
{
	my_video = (MyVideo *) calloc(1,sizeof(MyVideo));
	my_video->showVideo = showVideo;
	my_video->hideVideo = hideVideo;
	my_video->faceStart = faceStart;
	my_video->faceStop = faceStop;
	my_video->faceRegist = faceRegist;
	my_video->faceDelete = faceDelete;
	my_video->capture = capture;
	my_video->recordStart = recordStart;
	my_video->recordStop = recordStop;
	my_video->videoCallOut = videoCallOut;
	my_video->videoCallOutAll = videoCallOutAll;
	my_video->videoCallIn = videoCallIn;
	my_video->videoHangup = videoHangup;
	my_video->videoAnswer = videoAnswer;
	init();
	stm = stateMachineCreate(ST_IDLE,
			state_table,
			sizeof (state_table) / sizeof ((state_table) [0]),
			0,
			stmHandle,
			&st_debug);
	createThread(videoTimerThread,NULL);
}
