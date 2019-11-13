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
#include "jpeg_enc_dec.h"
#include "media_muxer.h"
#include "my_face.h"
#include "state_machine.h"
#include "ucpaas/ucpaas.h"
#include "sql_handle.h"
#include "protocol.h"
#include "externfunc.h"
#include "thread_helper.h"
#include "timer.h"
#include "config.h"
#include "my_video.h"
#include "my_mixer.h"
#include "my_audio.h"
#include "video/video_server.h"
#include "share_memory.h"
#include "my_update.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern void resetAutoSleepTimerLong(void);
extern void resetAutoSleepTimerShort(void);
extern int formCreateCaputure(int count);

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define IAMGE_MAX_W 1280
#define IAMGE_MAX_H 720
#define IMAGE_MAX_DATA (IAMGE_MAX_W * IAMGE_MAX_H * 3 / 2 )


#define TIME_TALKING 180
#define TIME_CALLING 30
#define TIME_RECORD 30
enum {
	EV_FACE_ON,			// 打开人脸识别功能
	EV_FACE_OFF_FINISH,	// 关闭人脸识别功能结束
	EV_FACE_REGIST,		// 注册人脸
	EV_FACE_RECOGNIZER,	// 识别人脸
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
	EV_DELAY_SLEEP,		// APP操作时延长睡眠时间
	EV_UPDATE,			// 升级软件
	EV_UPDATE_FINISH,	// 升级完成
};

enum {
	ST_IDLE,		// 空闲状态
	ST_FACE,		// 人脸识别开启状态
	ST_FACEOFF_RECORD,		// 人脸识别关闭过程，之后开始录像
	ST_TALK_CALLOUT,// 对讲呼出状态
	ST_TALK_CALLOUTALL,// 遍历对讲呼出状态
	ST_TALK_CALLIN,	// 对讲呼入状态
	ST_TALK_TALKING,// 对讲中状态
	ST_RECORDING, 	// 录像状态
	ST_RECORD_STOPPING,// 录像停止状态
	ST_UPDATE,		// 升级状态
};

enum {
	DO_FAIL, 		// 信息发送失败
	DO_NOTHING, 	// 不做任何操作
	DO_FACE_ON, 	// 人脸识别开启
	DO_FACE_OFF, 	// 人脸识别关闭
    DO_FACE_REGIST, // 注册人脸
	DO_FACE_RECOGNIZER,	// 识别人脸
    DO_TALK_CALLOUT, 	// 对讲呼出
    DO_TALK_CALLOUTALL, // 遍历对讲呼出
    DO_TALK_CALLIN, 	// 对讲呼入
    DO_TALK_ANSWER, 	// 对讲接听
    DO_TALK_HANGUP, 	// 对讲挂机
    DO_TALK_HANGUPALL, 	// 遍历对讲挂机
    DO_CAPTURE, 		// 抓拍本地
    DO_CAPTURE_NO_UI, 	// 抓拍本地,不进入抓拍界面
    DO_RECORD_START, 	// 录像开始
    DO_RECORD_STOP, 	// 录像结束
	DO_DELAY_SLEEP,		// APP操作时延长睡眠时间
	DO_UPDATE,			// 执行升级操作
};

enum {
	CALL_RESULT_NO,		// 未得到呼叫结果或已处理过呼叫结果
	CALL_RESULT_SUCCESS, // 呼叫成功
	CALL_RESULT_FAIL,  // 呼叫失败
};

typedef struct _StmData {
	int type;
	int call_dir; // 操作方向 0 本机，1对方
	int cap_count; // 抓拍照片数量
	int cap_type; // 抓拍类型
	char nick_name[128];
	char usr_id[128];
	// 升级使用
	char ip[16];
	int port;
	char file_path[512];
}StmData;

typedef struct _TalkPeerDev {
	char peer_nick_name[128];
	int call_out_result;
	int call_time;
	int type;
}TalkPeerDev;

typedef struct _StmDo {
	int action;
	int (*proc)(void *data,MyVideo *arg);
}StmDo;

typedef struct _CapData {
	uint64_t pic_id;
	int count;
	char file_date[32];
	char file_name[32];
	char nick_name[128];
	char usr_id[128];
	int type;	//抓拍类型
}CapData;

typedef struct _CammerData {
	int get_data_end;
	int type;
	int w,h;
	char data[IMAGE_MAX_DATA];
}CammerData;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
MyVideo *my_video;
static char *st_debug_ev[] = {
	"EV_FACE_ON",			// 打开人脸识别功能
	"EV_FACE_OFF_FINISH",   // 关闭人脸识别功能结束
	"EV_FACE_REGIST",       // 注册人脸
	"EV_FACE_RECOGNIZER",	// 识别人脸
	"EV_TALK_CALLOUT",      // 对讲呼出
	"EV_TALK_CALLOUTALL",   // 遍历对讲呼出
	"EV_TALK_CALLOUTOK",	// 对讲呼出成功
	"EV_TALK_CALLIN",       // 对讲呼入
	"EV_TALK_ANSWER",       // 对讲接听
	"EV_TALK_HANGUP",       // 对讲挂机
	"EV_TALK_HANGUPALL",	// 遍历对讲挂机
	"EV_CAPTURE",           // 截图
	"EV_RECORD_START",      // 录像开始
	"EV_RECORD_STOP",       // 录像结束
	"EV_RECORD_STOP_FINISHED",// 录像结束后执行动作
	"EV_DELAY_SLEEP", 		// APP操作时延长睡眠时间
	"EV_UPDATE",			// 升级软件
	"EV_UPDATE_FINISH",		// 升级完成
};
static char *st_debug_st[] = {
	"ST_IDLE",
	"ST_FACE",
	"ST_FACEOFF_RECORD",		// 人脸识别关闭过程，之后开始录像
	"ST_TALK_CALLOUT",
	"ST_TALK_CALLOUTALL",
	"ST_TALK_CALLIN",
	"ST_TALK_TALKING",
	"ST_RECORDING",
	"ST_RECORD_STOPPING",
	"ST_UPDATE",		// 升级状态
};
static char *st_debug_do[] = {
	"DO_FAIL",
	"DO_NOTHING",
	"DO_FACE_ON",
	"DO_FACE_OFF",
    "DO_FACE_REGIST",
    "DO_FACE_RECOGNIZER",
    "DO_TALK_CALLOUT",
    "DO_TALK_CALLOUTALL",
    "DO_TALK_CALLIN",
    "DO_TALK_ANSWER",
    "DO_TALK_HANGUP",
    "DO_TALK_HANGUPALL", 	// 遍历对讲挂机
    "DO_CAPTURE",
	"DO_CAPTURE_NO_UI",
    "DO_RECORD_START",
    "DO_RECORD_STOP",
	"DO_DELAY_SLEEP",		// APP操作时延长睡眠时间
	"DO_UPDATE",			// 执行升级操作
};

static StateTableDebug st_debug = {
	.ev = st_debug_ev,
	.st = st_debug_st,
	.todo = st_debug_do,
};
static StmData *st_data = NULL;
static StMachine* stm;
static CapData cap_data;
static MPEG4Head* avi = NULL;
static ReportTalkData talk_data;

static TalkPeerDev talk_peer_dev;	// 对讲对方信息
static pthread_mutex_t mutex;
static ShareMemory *share_mem = NULL;  //共享内存
static int send_video_start = 0;
static int record_state = 0;	// 1录像状态 0非录像状态
static int record_time = 0;		// 录像倒计时时间

static StateTable state_table[] =
{
	// 人脸开启，注册，识别
	{EV_FACE_ON,		ST_IDLE,	ST_FACE,	DO_FACE_ON},
	{EV_FACE_REGIST,	ST_FACE,	ST_FACE,	DO_FACE_REGIST},
	{EV_FACE_RECOGNIZER,ST_FACE,	ST_FACE,	DO_FACE_RECOGNIZER},

	// 关闭人脸时，根据当前不同状态，做不同操作
	{EV_FACE_OFF_FINISH,ST_TALK_CALLOUT,	ST_TALK_CALLOUT,	DO_TALK_CALLOUT},
	{EV_FACE_OFF_FINISH,ST_TALK_CALLOUTALL,	ST_TALK_CALLOUTALL,	DO_TALK_CALLOUTALL},
	{EV_FACE_OFF_FINISH,ST_TALK_CALLIN,		ST_TALK_CALLIN,		DO_TALK_CALLIN},
	{EV_FACE_OFF_FINISH,ST_FACEOFF_RECORD,	ST_RECORDING,		DO_RECORD_START},
	{EV_FACE_OFF_FINISH,ST_UPDATE,			ST_UPDATE,			DO_UPDATE},

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
	{EV_RECORD_START,	ST_FACE,			ST_FACEOFF_RECORD,	DO_FACE_OFF},
	{EV_RECORD_START,	ST_TALK_CALLIN,		ST_TALK_CALLIN,		DO_RECORD_START},
	{EV_RECORD_START,	ST_TALK_CALLOUT,	ST_TALK_CALLOUT,	DO_RECORD_START},
	{EV_RECORD_START,	ST_TALK_TALKING,	ST_TALK_TALKING,	DO_RECORD_START},

	{EV_RECORD_STOP,	ST_RECORDING,		ST_RECORD_STOPPING,	DO_RECORD_STOP},
	{EV_RECORD_STOP,	ST_TALK_CALLIN,		ST_TALK_CALLIN,		DO_RECORD_STOP},
	{EV_RECORD_STOP,	ST_TALK_CALLOUT,	ST_TALK_CALLOUT,	DO_RECORD_STOP},
	{EV_RECORD_STOP,	ST_TALK_TALKING,	ST_TALK_TALKING,	DO_RECORD_STOP},

	{EV_RECORD_STOP_FINISHED,	ST_RECORD_STOPPING,	ST_FACE,	DO_FACE_ON},

	{EV_CAPTURE,	ST_IDLE,			ST_IDLE,			DO_CAPTURE},
	{EV_CAPTURE,	ST_FACE,			ST_FACE,			DO_CAPTURE},
	{EV_CAPTURE,	ST_TALK_TALKING,	ST_TALK_TALKING,	DO_CAPTURE_NO_UI},
	{EV_CAPTURE,	ST_TALK_CALLOUT,	ST_TALK_CALLOUT,	DO_CAPTURE_NO_UI},
	{EV_CAPTURE,	ST_TALK_CALLOUTALL,	ST_TALK_CALLOUTALL,	DO_CAPTURE_NO_UI},
	{EV_CAPTURE,	ST_TALK_CALLIN,		ST_TALK_CALLIN,		DO_CAPTURE_NO_UI},
	
	{EV_DELAY_SLEEP,ST_IDLE,	ST_IDLE,	DO_DELAY_SLEEP},
	{EV_DELAY_SLEEP,ST_FACE,	ST_FACE,	DO_DELAY_SLEEP},

	{EV_UPDATE,		ST_FACE,	ST_UPDATE,	DO_FACE_OFF},
	{EV_UPDATE,		ST_IDLE,	ST_UPDATE,	DO_UPDATE},
};

static int stmDoFail(void *data,MyVideo *arg)
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
static int stmDoNothing(void *data,MyVideo *arg)
{
	
}

static int stmDoFaceOn(void *data,MyVideo *arg)
{
#ifdef USE_VIDEO
    rkVideoFaceOnOff(1);
#endif
}

static int stmDoFaceOff(void *data,MyVideo *arg)
{
#ifdef USE_VIDEO
	printf("face off\n");
    rkVideoFaceOnOff(0);
	printf("face off end\n");

#endif
	st_data = (StmData *)stm->initPara(stm,
			        sizeof(StmData));
	if(data)
		memcpy(st_data,data,sizeof(StmData));
	stm->msgPost(stm,EV_FACE_OFF_FINISH,st_data);
}

static int stmDoFaceRegist(void *data,MyVideo *arg)
{
	if (my_face)
		return my_face->regist((MyFaceRegistData *)data);
	else
		return -1;
}
static int stmDoFaceRecognizer(void *data,MyVideo *arg)
{
	if (my_face)
		return my_face->recognizerOnce((MyFaceRecognizer *)data);
	else
		return -1;
}


#ifdef USE_VIDEO
static void sendVideoCallbackFunc(void *data,int size,int fram_type)
{
    protocol_talk->sendVideo(data,size);
}
#else
static void* sendVideoCallbackFunc(void *arg)
{
	send_video_start = 1;
	if (share_mem == NULL)
		share_mem = shareMemoryCreateSlave(1024*50,4);
	if (share_mem == NULL) {
		printf("[%s]main:share mem create fail\n",__func__);
		return NULL;
	}
	while (send_video_start) {
		int mem_len = 0;
		char *mem_data = (char *)share_mem->GetStart(share_mem,&mem_len);
		if (!mem_data || mem_len == 0) {
			share_mem->GetEnd(share_mem);
			goto send_sleep;
		}
		protocol_talk->sendVideo(mem_data,mem_len);
		share_mem->GetEnd(share_mem);
send_sleep:
		usleep(10000);
	}
	share_mem->CloseMemory(share_mem);
	share_mem->Destroy(share_mem);
	share_mem = NULL;
	return NULL;
}
#endif
static void dialCallBack(int result)
{
	if (result) {
		talk_peer_dev.call_out_result = CALL_RESULT_SUCCESS;
		stm->msgPost(stm,EV_TALK_CALLOUTOK,NULL);
		if (		talk_peer_dev.type != DEV_TYPE_ENTRANCEMACHINE
				&& 	talk_peer_dev.type != DEV_TYPE_HOUSEENTRANCEMACHINE) {
#ifdef USE_VIDEO
			rkH264EncOn(320,240,sendVideoCallbackFunc);
#endif
		}
	} else {
		talk_peer_dev.call_out_result = CALL_RESULT_FAIL;
		stm->msgPost(stm,EV_TALK_HANGUP,NULL);
	}
}
static int stmDoTalkCallout(void *data,MyVideo *arg)
{
	StmData *data_temp = (StmData *)data;
	char ui_title[128] = {0};
	// TODO 临时写死门口机Ip
	if (strcmp(data_temp->usr_id,"172.16.5.3")) {
		int ret = sqlGetUserInfoUseUserId(data_temp->usr_id,data_temp->nick_name,&data_temp->type);
		if (ret == 0) {
			printf("can't find usr_id:%s\n", data_temp->usr_id);
			stm->msgPost(stm,EV_TALK_HANGUP,NULL);
			return -1;
		}
		sprintf(ui_title,"正在呼叫 %s",data_temp->nick_name);
		talk_data.call_dir = CALL_DIR_OUT;
		if (protocol_talk->uiShowFormVideo)
			protocol_talk->uiShowFormVideo(data_temp->type,ui_title,talk_data.call_dir);
		talk_peer_dev.type = data_temp->type;
		talk_peer_dev.call_time = TIME_CALLING;
		protocol_talk->dial(data_temp->usr_id,dialCallBack);
		if (talk_peer_dev.type == DEV_TYPE_ENTRANCEMACHINE)
			my_video->showPeerVideo();	
	} else {
		sprintf(ui_title,"正在呼叫 门口机");
		talk_peer_dev.type = DEV_TYPE_ENTRANCEMACHINE;
		talk_peer_dev.call_time = TIME_CALLING;
		if (protocol_talk->uiShowFormVideo)
			protocol_talk->uiShowFormVideo(DEV_TYPE_ENTRANCEMACHINE,ui_title,talk_data.call_dir);
		protocol_talk->dial(data_temp->usr_id,dialCallBack);
		my_video->showPeerVideo();	
	}
	// 保存通话记录到内存
	strcpy(talk_data.nick_name,data_temp->nick_name);
	getDate(talk_data.date,sizeof(talk_data.date));
}

static void *threadCallOutAll(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	char user_id[128] = {0};
	int index = 0;
	int talk_online_times = 10 * 10; // 判断云对讲是否连上服务，超过10S未连上服务，则不呼叫
	int user_num = sqlGetUserInfoUseScopeStart(DEV_TYPE_HOUSEHOLDAPP);
	sqlGetUserInfoEnd();
	protocol_talk->type = PROTOCOL_TALK_CLOUD;
	while (user_num) {
#ifdef USE_UCPAAS
		if (ucsConnectState() == 0) {
			if (talk_online_times) {
				if (--talk_online_times == 0) {
					user_num = 0;
					break;
				}
			}
			usleep(100000);
			continue;
		}
#endif
		sqlGetUserInfosUseScopeIndex(user_id,DEV_TYPE_HOUSEHOLDAPP,index++);
		my_video->videoCallOut(user_id);
		printf("[%s]id:%s,name:%s\n", __func__,user_id,talk_peer_dev.peer_nick_name);
		int wait_times = 300;
		while (wait_times) {
			if (talk_peer_dev.call_out_result == CALL_RESULT_NO) {
				usleep(10000);
				wait_times--;
				continue;
			}
			if (talk_peer_dev.call_out_result == CALL_RESULT_SUCCESS) {
				talk_peer_dev.call_out_result = CALL_RESULT_NO;
				return NULL;
			}
			if (talk_peer_dev.call_out_result == CALL_RESULT_FAIL) {
				talk_peer_dev.call_out_result = CALL_RESULT_NO;
				break;
			}
		}
		user_num--;
	}
	if (user_num == 0)
		stm->msgPost(stm,EV_TALK_HANGUPALL,NULL);

	return NULL;
}
static int stmDoTalkCalloutAll(void *data,MyVideo *arg)
{
	createThread(threadCallOutAll,NULL);
}

static int stmDoTalkCallin(void *data,MyVideo *arg)
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
	// 3000局域网对讲默认为门口机
	if (protocol_talk->type == PROTOCOL_TALK_LAN) {
		data_temp->type = DEV_TYPE_ENTRANCEMACHINE;
	}

	talk_data.call_dir = CALL_DIR_IN;
	if (protocol_talk->uiShowFormVideo)
		protocol_talk->uiShowFormVideo(data_temp->type,ui_title,talk_data.call_dir);

	// 保存通话记录到内存
	strcpy(talk_data.nick_name,data_temp->nick_name);
	getDate(talk_data.date,sizeof(talk_data.date));

	talk_peer_dev.type = data_temp->type;
	talk_peer_dev.call_time = TIME_CALLING;
	if (data_temp->type == DEV_TYPE_HOUSEHOLDAPP) {
		my_video->videoAnswer(0,data_temp->type);
	} else {
		myAudioPlayRing();
		my_video->showPeerVideo();	
	}
	return 0;
}
static int stmDoTalkAnswer(void *data,MyVideo *arg)
{
	char ui_title[128] = {0};
	if (		talk_peer_dev.type != DEV_TYPE_ENTRANCEMACHINE
			&& 	talk_peer_dev.type != DEV_TYPE_HOUSEENTRANCEMACHINE) {
#ifdef USE_VIDEO
		rkH264EncOn(320,240,sendVideoCallbackFunc);
#endif
	}
	memset(ui_title,0,sizeof(ui_title));
	StmData *data_temp = (StmData *)data;
	sprintf(ui_title,"正在与 %s 通话",talk_peer_dev.peer_nick_name);
	if (talk_peer_dev.type == DEV_TYPE_HOUSEHOLDAPP) {
		screensaverSet(0);
	}
	protocol_talk->answer();
	if (protocol_talk->uiAnswer)
		protocol_talk->uiAnswer(ui_title);
	talk_peer_dev.call_time = TIME_TALKING;
	talk_data.answered = 1;
}

static int stmDoTalkHangupAll(void *data,MyVideo *arg)
{
	protocol_talk->hangup();
}

static int stmDoTalkHangup(void *data,MyVideo *arg)
{
#ifdef USE_VIDEO
	rkH264EncOff();
#endif
	protocol_talk->hangup();
	if (protocol_talk->uiHangup)
		protocol_talk->uiHangup();
	if (talk_data.answered) {
		talk_data.talk_time = TIME_TALKING - talk_peer_dev.call_time;
	} else {
		talk_data.talk_time = TIME_CALLING - talk_peer_dev.call_time;
	}
	sqlInsertRecordTalkNoBack(talk_data.date,
			talk_data.nick_name,
			talk_data.call_dir,
			talk_data.answered,
			talk_data.talk_time,
			talk_data.picture_id);
	protocol_hardcloud->uploadPic(FAST_PIC_PATH,talk_data.picture_id);
	protocol_hardcloud->reportTalk(&talk_data);
	memset(&talk_data,0,sizeof(talk_data));
	memset(&talk_peer_dev,0,sizeof(talk_peer_dev));
	record_time = 0;
}

static void* threadCapture(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	CapData cap_data_temp;
	memcpy(&cap_data_temp,arg,sizeof(CapData));
	int i;
	char jpg_name[64] = {0};
	char file_path[64] = {0};
	char url[256] = {0};
	// printf("thread count:%d,date:%s,name:%s\n",cap_data_temp.count,cap_data_temp.file_date,cap_data_temp.file_name);
	for (i=0; i<cap_data_temp.count; i++) {
		sprintf(jpg_name,"%s_%s_%d.jpg",g_config.imei,cap_data_temp.file_name,i);
		sprintf(file_path,"%s%s",FAST_PIC_PATH,jpg_name);
		// printf("wirte :%s\n",file_path);

#ifdef USE_VIDEO
		rkVideoCapture(file_path);
#ifdef X86
		FILE *fp = fopen(file_path,"wb");
		if (fp)
			fclose(fp);
#endif
#endif
		sprintf(url,"%s/%s",QINIU_URL,jpg_name);
		sqlInsertPicUrlNoBack(cap_data_temp.pic_id,url);
		usleep(500000);
	}
	sleep(1);
	if (cap_data_temp.type == CAP_TYPE_TALK)
		talk_data.picture_id = cap_data_temp.pic_id;
	else {
		protocol_hardcloud->uploadPic(FAST_PIC_PATH,cap_data_temp.pic_id);
		protocol_hardcloud->reportCapture(cap_data_temp.pic_id);
	}
	return NULL;
}

static void* threadAlarm(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	CapData cap_data_temp;
	memcpy(&cap_data_temp,arg,sizeof(CapData));

	int i;
	char jpg_name[64] = {0};
	char file_path[64] = {0};
	char url[256] = {0};
	static ReportAlarmData alarm_data;
	alarm_data.type = ALARM_TYPE_PEOPLES;
	alarm_data.picture_id = cap_data_temp.pic_id;
	alarm_data.has_people = 0;
	FILE *fp = NULL;
	for (i=0; i<cap_data_temp.count; i++) {
		sprintf(jpg_name,"%s_%s_%d.jpg",g_config.imei,cap_data_temp.file_name,i);
		sprintf(file_path,"%s%s",FAST_PIC_PATH,jpg_name);
#ifdef USE_VIDEO
		rkVideoCapture(file_path);
#endif
		// wait for write file
		usleep(500000);

		char pic_buf_jpg[100 * 1024] = {0};
		unsigned char *pic_buf_yuv = NULL;
		int yuv_len = 0;
		int w = 0,h = 0;
		int leng = 0;
		fp = fopen(file_path,"rb");
		if (fp) {
			leng = fread(pic_buf_jpg,1,sizeof(pic_buf_jpg),fp);
			fclose(fp);
			jpegToYuv420sp((unsigned char *)pic_buf_jpg, leng,&w,&h, &pic_buf_yuv, &yuv_len);
			if (my_video->faceRecognizer(pic_buf_yuv,w,h,&alarm_data.age,&alarm_data.sex) == 0)
				alarm_data.has_people = 1;
			sprintf(url,"%s/%s",QINIU_URL,jpg_name);
			sqlInsertPicUrlNoBack(cap_data_temp.pic_id,url);
		}
		if (pic_buf_yuv)
			free(pic_buf_yuv);
	}
	sqlInsertRecordAlarm(alarm_data.date,
			alarm_data.type,
			alarm_data.has_people,
			alarm_data.age,
			alarm_data.sex,
			alarm_data.picture_id);
	sleep(1);
	protocol_hardcloud->uploadPic(FAST_PIC_PATH,alarm_data.picture_id);
	protocol_hardcloud->reportAlarm(&alarm_data);
	myAudioPlayAlarm();
	return NULL;
}
static void* threadFace(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	CapData cap_data_temp;
	memcpy(&cap_data_temp,arg,sizeof(CapData));

	int i;
	char file_path[64] = {0};
	char jpg_name[64] = {0};
	char url[256] = {0};
	static ReportFaceData face_data;
	
	strcpy(face_data.date,cap_data_temp.file_date);
	strcpy(face_data.nick_name,cap_data_temp.nick_name);
	strcpy(face_data.user_id,cap_data_temp.usr_id);
	face_data.picture_id = cap_data_temp.pic_id;

	for (i=0; i<cap_data_temp.count; i++) {
		sprintf(jpg_name,"%s_%s_%d.jpg",g_config.imei,cap_data_temp.file_name,i);
		sprintf(file_path,"%s%s",FAST_PIC_PATH,jpg_name);
#ifdef USE_VIDEO
		rkVideoCapture(file_path);
#endif
		sprintf(url,"%s/%s",QINIU_URL,jpg_name);
		sqlInsertPicUrlNoBack(cap_data_temp.pic_id,url);

		// wait for write file
		usleep(500000);

	}

	sqlInsertRecordFaceNoBack(face_data.date,
			face_data.user_id,
			face_data.nick_name,
			face_data.picture_id);
	sleep(1);
	protocol_hardcloud->uploadPic(FAST_PIC_PATH,face_data.picture_id);
	protocol_hardcloud->reportFace(&face_data);
	return NULL;
}
static int stmDoCaptureNoUi(void *data,MyVideo *arg)
{
	StmData *data_temp = (StmData *)data;
	memset(&cap_data,0,sizeof(CapData));
	getFileName(cap_data.file_name,cap_data.file_date);
	cap_data.pic_id = atoll(cap_data.file_name);
	cap_data.count = data_temp->cap_count;
	cap_data.type = data_temp->cap_type;
	switch(data_temp->cap_type)
	{
		case CAP_TYPE_FORMMAIN :
		case CAP_TYPE_DOORBELL :
			sqlInsertRecordCapNoBack(cap_data.file_date,cap_data.pic_id);
		case CAP_TYPE_TALK :
			createThread(threadCapture,&cap_data);
			break;
		case CAP_TYPE_ALARM :
			createThread(threadAlarm,&cap_data);
			break;
		case CAP_TYPE_FACE :
			if (data_temp->nick_name)
				strcpy(cap_data.nick_name,data_temp->nick_name);
			if (data_temp->usr_id)
				strcpy(cap_data.usr_id,data_temp->usr_id);
			createThread(threadFace,&cap_data);
			break;
		default:
			break;
	}
}
static int stmDoCapture(void *data,MyVideo *arg)
{
	StmData *data_temp = (StmData *)data;
	stmDoCaptureNoUi(data,arg);
	formCreateCaputure(cap_data.count);
}

static void recordVideoCallbackFunc(void *data,int size,int fram_type)
{
    if (avi == NULL)
        return ;
    // 从关键帧开始写视频
    if (avi->FirstFrame) {
        if (fram_type == 1) {
            avi->FirstFrame = 0;
            avi->WriteVideo(avi,data,size);
        }
    } else {
        avi->WriteVideo(avi,data,size);
    }
}

static void recordStopCallbackFunc(void)
{
	printf("[%s]\n", __func__);
    pthread_mutex_lock(&mutex);
    if (avi) 
        avi->DestoryMPEG4(&avi);
    pthread_mutex_unlock(&mutex);
	protocol_hardcloud->uploadPic(FAST_PIC_PATH,cap_data.pic_id);
	protocol_hardcloud->reportCapture(cap_data.pic_id);
}

#if (defined X86)
static void* threadAviReadVideo(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	int i;
	for (i=0; i<50; i++) {
		char buf[100];
		sprintf(buf,"%02d",i);
		if (i == 0)
			recordVideoCallbackFunc(buf,2,1);
		else
			recordVideoCallbackFunc(buf,2,0);
		usleep(100000);
	}
	recordStopCallbackFunc();
	stm->msgPost(stm,EV_RECORD_STOP_FINISHED,NULL);
}
#endif
static void* threadAviReadAudio(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
    int audio_fp = -1;
    avi->InitAudio(avi,2,8000,1024);
	if (my_mixer)
		my_mixer->InitPlayAndRec(my_mixer,&audio_fp,8000,2);
    while (avi) {
		int real_size = 0;
        char audio_buff[1024] = {0};
		if (my_mixer)
			real_size = my_mixer->Read(my_mixer,audio_buff,sizeof(audio_buff),2);
        pthread_mutex_lock(&mutex);
        if (avi)
            avi->WriteAudio(avi,audio_buff,real_size);
        pthread_mutex_unlock(&mutex);
        usleep(10000);
    }
	if (my_mixer) {
		if (audio_fp > 0)
			my_mixer->DeInitPlay(my_mixer,&audio_fp);
	}
    return NULL; 
}
static int stmDoRecordStart(void *data,MyVideo *arg)
{
	if (record_state == 1)
		return 0;
	record_state = 1;
	StmData *data_temp = (StmData *)data;
	memset(&cap_data,0,sizeof(CapData));
	getFileName(cap_data.file_name,cap_data.file_date);
	cap_data.pic_id = atoll(cap_data.file_name);
	int w = 320,h = 240;
	record_time = TIME_RECORD;
	switch(data_temp->cap_type)
	{
		case CAP_TYPE_FORMMAIN :
			{
				w = 640;
				h = 480;
			}
		case CAP_TYPE_TALK :
            {
                char file_path[64] = {0};
                char url[256] = {0};
				char jpg_name[64] = {0};
                sqlInsertRecordCapNoBack(cap_data.file_date,cap_data.pic_id);
				sprintf(jpg_name,"%s_%s.mp4",g_config.imei,cap_data.file_name);
				// sprintf(file_path,"/temp/%s",jpg_name);
				sprintf(file_path,"%s%s",FAST_PIC_PATH,jpg_name);
                if (avi == NULL) {
                    avi = Mpeg4_Create(w,h,file_path,WRITE_READ);
					if (data_temp->cap_type == CAP_TYPE_FORMMAIN)
						createThread(threadAviReadAudio,NULL);
					else 
						avi->InitAudio(avi,2,8000,0);
                }
#ifdef USE_VIDEO
                rkVideoRecordStart(w,h,recordVideoCallbackFunc);
                rkVideoRecordSetStopFunc(recordStopCallbackFunc);
#endif
#ifdef X86
				createThread(threadAviReadVideo,NULL);
#endif
				sprintf(url,"%s/%s",QINIU_URL,jpg_name);
                sqlInsertRecordUrlNoBack(cap_data.pic_id,url);
            }
			break;
		case CAP_TYPE_ALARM :
			{
				printf("file_name\n");
			}
			break;
		default:
			break;
	}
}
static int stmDoRecordStop(void *data,MyVideo *arg)
{
	if (record_state == 0)
		return 0;
	record_state = 0;
#ifdef USE_VIDEO
    rkVideoRecordStop();
	// 若正在通话，则不关闭h264编码
	if (stm->getCurrentstate(stm) == ST_RECORD_STOPPING) {
		rkH264EncOff();
		if (protocol_talk->uiHangup)
			protocol_talk->uiHangup();
	}
#endif
	record_time = 0;
	stm->msgPost(stm,EV_RECORD_STOP_FINISHED,NULL);
}

static int stmDoDelaySleepTime(void *data,MyVideo *arg)
{
	StmData *data_temp = (StmData *)data;
	if (data_temp->type) {
		resetAutoSleepTimerLong();
	} else {
		resetAutoSleepTimerShort();
	}
	
}

static int stmDoUpdate(void *data,MyVideo *arg)
{
	StmData *data_temp = (StmData *)data;
	myUpdateStart(data_temp->type,data_temp->ip,data_temp->port,data_temp->file_path);
}

static StmDo stm_do[] =
{
	{DO_FAIL,			stmDoFail},
	{DO_NOTHING,		stmDoNothing},
	{DO_FACE_ON,    	stmDoFaceOn},
	{DO_FACE_OFF,   	stmDoFaceOff},
	{DO_FACE_REGIST,	stmDoFaceRegist},
	{DO_FACE_RECOGNIZER,stmDoFaceRecognizer},
	{DO_TALK_CALLOUT,	stmDoTalkCallout},
	{DO_TALK_CALLOUTALL,stmDoTalkCalloutAll},
	{DO_TALK_CALLIN,	stmDoTalkCallin},
	{DO_TALK_ANSWER,	stmDoTalkAnswer},
	{DO_TALK_HANGUP,	stmDoTalkHangup},
	{DO_TALK_HANGUPALL,	stmDoTalkHangupAll},
	{DO_CAPTURE,		stmDoCapture},
	{DO_CAPTURE_NO_UI,	stmDoCaptureNoUi},
	{DO_RECORD_START,	stmDoRecordStart},
	{DO_RECORD_STOP,	stmDoRecordStop},
	{DO_DELAY_SLEEP,	stmDoDelaySleepTime},
	{DO_UPDATE,			stmDoUpdate},

};

static int stmHandle(StMachine *This,int result,void *data,void *arg)
{
	if (result) {
		return stm_do[This->getCurRun(This)].proc(data,(MyVideo *)arg);
	} else {
		return stm_do[DO_FAIL].proc(data,(MyVideo *)arg);
	}
}

static void* threadVideoInit(void *arg)
{
	while (access(IPC_CAMMER,0) == 0) {
		usleep(10000);
	}
#ifdef USE_VIDEO
	rkVideoInit();
#endif
	return NULL;
}
static void init(void)
{
	jpegIncDecInit();
	myFaceInit();
	createThread(threadVideoInit,NULL);
}

static void capture(int type,int count,char *nick_name,char *user_id)
{
	st_data = (StmData *)stm->initPara(stm,
			        sizeof(StmData));
	st_data->cap_count = count;
	st_data->cap_type = type;
	if (nick_name)
		strcpy(st_data->nick_name,nick_name);
	if (user_id)
		strcpy(st_data->usr_id,user_id);
	stm->msgPost(stm,EV_CAPTURE,st_data);
}

static void recordStart(int type)
{
	st_data = (StmData *)stm->initPara(stm,
			        sizeof(StmData));
	st_data->cap_type = type;
    stm->msgPost(stm,EV_RECORD_START,st_data);
}

static void recordStop(void)
{
    stm->msgPost(stm,EV_RECORD_STOP,NULL);
}
static void recordWriteCallback(char *data,int size)
{
	if (avi && avi->FirstFrame == 0) {
        avi->WriteAudio(avi,data,size);
	}
}

static void videoCallOut(char *user_id)
{
	int scope = 0;
	st_data = (StmData *)stm->initPara(stm,
			        sizeof(StmData));
	strcpy(st_data->usr_id,user_id);
	stm->msgPost(stm,EV_TALK_CALLOUT,st_data);
	sqlGetUserInfoUseUserId(user_id,talk_peer_dev.peer_nick_name,&scope);
	talk_peer_dev.call_out_result = CALL_RESULT_NO;
}
static int videoGetCallTime(void)
{
	return talk_peer_dev.call_time;
}

static int videoGetRecordTime(void)
{
	return record_time;
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
static void* threadFaceOnDelay(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	sleep(4);
    stm->msgPost(stm,EV_FACE_ON,NULL);
	return NULL;
}
static void showLocalVideo(void)
{
#ifdef USE_VIDEO
	rkVideoDisplayLocal();
#endif
	createThread(threadFaceOnDelay,NULL);
    // stm->msgPost(stm,EV_FACE_ON,NULL);
}
static void receiveVideo(void *data,int *size)
{
	protocol_talk->receiveVideo(data,size);
}

static void showPeerVideo(void)
{
#ifdef USE_VIDEO
	rkVideoDisplayPeer(1024,600,receiveVideo);
#endif
}
static void hideVideo(void)
{
#ifdef USE_VIDEO
	rkVideoDisplayOff();
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

static int faceRecognizer( unsigned char *image_buff,int w,int h,int *age,int *sex)
{
    MyFaceRecognizer face_data;
    face_data.image_buff = image_buff;
    face_data.w = w;
    face_data.h = h;
	int ret = stm->msgPostSync(stm,EV_FACE_RECOGNIZER,&face_data);
	if (ret == 0) {
		*age = face_data.age;	
		*sex = face_data.sex;	
	}
    return ret;
}
static int delaySleepTime(int type) // 延长睡眠时间0短 1长
{
	st_data = (StmData *)stm->initPara(stm,
			        sizeof(StmData));
	st_data->type = type;
    stm->msgPost(stm,EV_DELAY_SLEEP,st_data);
}

static int update(int type,char *ip,int port,char *file_path)
{
	st_data = (StmData *)stm->initPara(stm,
			        sizeof(StmData));
	st_data->type = type;
	st_data->port = port;
	strcpy(st_data->ip,ip);
	strcpy(st_data->file_path,file_path);
    stm->msgPost(stm,EV_UPDATE,st_data);
}
static int isVideoOn(void)
{
	if (stm->getCurrentstate(stm) == ST_TALK_CALLOUT
			|| stm->getCurrentstate(stm) == ST_TALK_CALLOUTALL
			|| stm->getCurrentstate(stm) == ST_TALK_CALLIN
			|| stm->getCurrentstate(stm) == ST_TALK_TALKING)	
		return 1;
	else
		return 0;
}

static void* threadVideoTimer(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	while (1) {
		if (talk_peer_dev.call_time) {
			// printf("call time:%d\n", talk_peer_dev.call_time);
			if (--talk_peer_dev.call_time == 0) {
				stm->msgPost(stm,EV_TALK_HANGUP,NULL);
			}
		}
		if (record_time) {
			if (--record_time == 0) {
				stm->msgPost(stm,EV_RECORD_STOP,NULL);
			}
		}
		sleep(1);
	}
	return NULL;
}
void myVideoInit(void)
{
	my_video = (MyVideo *) calloc(1,sizeof(MyVideo));
	my_video->showLocalVideo = showLocalVideo;
	my_video->showPeerVideo = showPeerVideo;
	my_video->hideVideo = hideVideo;
	my_video->faceRegist = faceRegist;
	my_video->faceDelete = faceDelete;
	my_video->faceRecognizer = faceRecognizer;
	my_video->capture = capture;
	my_video->recordStart = recordStart;
	my_video->recordStop = recordStop;
	my_video->videoCallOut = videoCallOut;
	my_video->videoCallOutAll = videoCallOutAll;
	my_video->videoCallIn = videoCallIn;
	my_video->videoHangup = videoHangup;
	my_video->videoAnswer = videoAnswer;
	my_video->videoGetCallTime = videoGetCallTime;
	my_video->videoGetRecordTime = videoGetRecordTime;
    my_video->recordWriteCallback = recordWriteCallback;

    my_video->delaySleepTime = delaySleepTime;
    my_video->update = update;
    my_video->isVideoOn = isVideoOn;

	memset(&talk_data,0,sizeof(talk_data));
    pthread_mutexattr_t mutexattr;
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);
	init();
	stm = stateMachineCreate(ST_IDLE,
			state_table,
			sizeof (state_table) / sizeof ((state_table) [0]),
			0,
			stmHandle,
			my_video,
			&st_debug);
	createThread(threadVideoTimer,NULL);
}
