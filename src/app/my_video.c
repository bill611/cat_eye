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
#include "avi_encode.h"
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
#include "video/video_server.h"
#include "share_memory.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern void resetAutoSleepTimerLong(void);
extern void resetAutoSleepTimerShort(void);
extern int formCreateCaputure(int count);
extern IpcServer* ipc_main;

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
enum {
	EV_FACE_ON,			// 打开人脸识别功能
	EV_FACE_OFF,		// 关闭人脸识别功能
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
};

enum {
	ST_IDLE,		// 空闲状态
	ST_FACE,		// 人脸识别开启状态
	ST_TALK_CALLOUT,// 对讲呼出状态
	ST_TALK_CALLOUTALL,// 遍历对讲呼出状态
	ST_TALK_CALLIN,	// 对讲呼入状态
	ST_TALK_TALKING,// 对讲中状态
	ST_RECORDING, 	// 录像状态
	ST_RECORD_STOPPING,// 录像停止状态
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
};

typedef struct _StmData {
	int type;
	int call_dir; // 操作方向 0 本机，1对方
	int cap_count; // 抓拍照片数量
	int cap_type; // 抓拍类型
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
	int (*proc)(void *data,MyVideo *arg);
}StmDo;

typedef struct _CapData {
	uint64_t pic_id;
	int count;
	char file_date[32];
	char file_name[32];
	char nick_name[128];
	char usr_id[128];
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
	"EV_FACE_OFF",          // 关闭人脸识别功能
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
};
static char *st_debug_st[] = {
	"ST_IDLE",
	"ST_FACE",
	"ST_TALK_CALLOUT",
	"ST_TALK_CALLOUTALL",
	"ST_TALK_CALLIN",
	"ST_TALK_TALKING",
	"ST_RECORDING",
	"ST_RECORD_STOPPING",
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

static TalkPeerDev talk_peer_dev;
static pthread_mutex_t mutex;
static ShareMemory *share_mem = NULL;  //共享内存
static int send_video_start = 0;
static int rec_video_start = 0;

static StateTable state_table[] =
{
	{EV_FACE_ON,		ST_IDLE,	ST_FACE,	DO_FACE_ON},
	{EV_FACE_OFF,		ST_FACE,	ST_IDLE,	DO_FACE_OFF},
	{EV_FACE_REGIST,	ST_FACE,	ST_FACE,	DO_FACE_REGIST},
	{EV_FACE_RECOGNIZER,ST_FACE,	ST_FACE,	DO_FACE_RECOGNIZER},

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

	{EV_RECORD_STOP,	ST_RECORDING,		ST_RECORD_STOPPING,	DO_RECORD_STOP},
	{EV_RECORD_STOP,	ST_TALK_CALLIN,		ST_TALK_CALLIN,		DO_RECORD_STOP},
	{EV_RECORD_STOP,	ST_TALK_CALLOUT,	ST_TALK_CALLOUT,	DO_RECORD_STOP},
	{EV_RECORD_STOP,	ST_TALK_TALKING,	ST_TALK_TALKING,	DO_RECORD_STOP},

	{EV_RECORD_STOP_FINISHED,	ST_RECORD_STOPPING,	ST_FACE,	DO_FACE_ON},

	{EV_CAPTURE,	ST_IDLE,			ST_IDLE,		DO_CAPTURE},
	{EV_CAPTURE,	ST_FACE,			ST_FACE,		DO_CAPTURE},
	{EV_CAPTURE,	ST_TALK_TALKING,	ST_TALK_TALKING,DO_CAPTURE_NO_UI},
	
	{EV_DELAY_SLEEP,ST_IDLE,	ST_IDLE,	DO_DELAY_SLEEP},
	{EV_DELAY_SLEEP,ST_FACE,	ST_FACE,	DO_DELAY_SLEEP},
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
    rkVideoFaceOnOff(0);
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
		talk_peer_dev.call_out_result = 1;
		stm->msgPost(stm,EV_TALK_CALLOUTOK,NULL);
		if (		talk_peer_dev.type != DEV_TYPE_ENTRANCEMACHINE
				&& 	talk_peer_dev.type != DEV_TYPE_HOUSEENTRANCEMACHINE) {
#ifdef USE_VIDEO
			rkH264EncOn(320,240,sendVideoCallbackFunc);
#else
			IpcData ipc_data;
			ipc_data.dev_type = IPC_DEV_TYPE_MAIN;
			ipc_data.cmd = IPC_VIDEO_ENCODE_ON;
			if (ipc_main)
				ipc_main->sendData(ipc_main,IPC_CAMMER,&ipc_data,sizeof(ipc_data));
			createThread(sendVideoCallbackFunc,NULL);
#endif
		}
	} else {
		talk_peer_dev.call_out_result = 0;
		stm->msgPost(stm,EV_TALK_HANGUP,NULL);
	}
}
static int stmDoTalkCallout(void *data,MyVideo *arg)
{
#ifdef USE_UCPAAS
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
	talk_peer_dev.type = data_temp->type;
	talk_peer_dev.call_time = TIME_CALLING;
	protocol_talk->dial(data_temp->usr_id,dialCallBack);
#endif
#ifdef USE_UDPTALK
	StmData *data_temp = (StmData *)data;
	char ui_title[128] = {0};
	sprintf(ui_title,"正在呼叫 门口机");
	talk_peer_dev.type = DEV_TYPE_ENTRANCEMACHINE;
	talk_peer_dev.call_time = TIME_CALLING;
	if (protocol_talk->uiShowFormVideo)
		protocol_talk->uiShowFormVideo(DEV_TYPE_ENTRANCEMACHINE,ui_title);
	protocol_talk->dial(data_temp->usr_id,dialCallBack);
	my_video->showPeerVideo();	
#endif
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
		int wait_times = 300;
		while (wait_times) {
			if (talk_peer_dev.call_out_result == -1) {
				usleep(10000);
				wait_times--;
				continue;
			}
			if (talk_peer_dev.call_out_result == 1) {
				talk_peer_dev.call_out_result = -1;
				return NULL;
			}
			if (talk_peer_dev.call_out_result == 0) {
				talk_peer_dev.call_out_result = -1;
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
	if (protocol_talk->type == PROTOCOL_TALK_3000) {
		data_temp->type = DEV_TYPE_ENTRANCEMACHINE;
	}
	if (protocol_talk->uiShowFormVideo)
		protocol_talk->uiShowFormVideo(data_temp->type,ui_title);

	talk_peer_dev.type = data_temp->type;
	talk_peer_dev.call_time = TIME_CALLING;
	if (data_temp->type == DEV_TYPE_HOUSEHOLDAPP) {
		my_video->videoAnswer(0,data_temp->type);
	} else {
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
#else
		IpcData ipc_data;
		ipc_data.dev_type = IPC_DEV_TYPE_MAIN;
		ipc_data.cmd = IPC_VIDEO_ENCODE_ON;
		if (ipc_main)
			ipc_main->sendData(ipc_main,IPC_CAMMER,&ipc_data,sizeof(ipc_data));
		createThread(sendVideoCallbackFunc,NULL);
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

static int stmDoTalkHangupAll(void *data,MyVideo *arg)
{
#ifdef USE_VIDEO
	rkH264EncOff();
#endif
	protocol_talk->hangup();
	if (protocol_talk->uiHangup)
		protocol_talk->uiHangup();
	talk_peer_dev.call_time = 0;
	memset(&talk_peer_dev,0,sizeof(talk_peer_dev));
	talk_peer_dev.call_out_result = -1;
}

static int stmDoTalkHangup(void *data,MyVideo *arg)
{
	stmDoTalkHangupAll(data,arg);
	// my_video->hideVideo();
}

static void* threadCapture(void *arg)
{
	CapData cap_data_temp;
	memcpy(&cap_data_temp,arg,sizeof(CapData));
	int i;
	char file_path[64] = {0};
	char url[256] = {0};
	// printf("thread count:%d,date:%s,name:%s\n",cap_data_temp.count,cap_data_temp.file_date,cap_data_temp.file_name);
	for (i=0; i<cap_data_temp.count; i++) {
		sprintf(file_path,"%s%s_%d.jpg",FAST_PIC_PATH,cap_data_temp.file_name,i);
		// printf("wirte :%s\n",file_path);

#ifdef USE_VIDEO
		rkVideoCapture(file_path);
#else
        IpcData ipc_data;
        ipc_data.dev_type = IPC_DEV_TYPE_MAIN;
        ipc_data.cmd = IPC_VIDEO_CAPTURE;
        strcpy(ipc_data.data.cap_path,file_path);
        if (ipc_main)
            ipc_main->sendData(ipc_main,IPC_CAMMER,&ipc_data,sizeof(ipc_data));
#ifdef X86
		FILE *fp = fopen(file_path,"wb");
		fclose(fp);
#endif
#endif
		sprintf(url,"%s/%s_%d.jpg",QINIU_URL,cap_data_temp.file_name,i);
		sqlInsertPicUrlNoBack(cap_data_temp.pic_id,url);
		usleep(500000);
	}
	protocol_hardcloud->uploadPic(FAST_PIC_PATH,cap_data_temp.pic_id);
	protocol_hardcloud->reportCapture(cap_data_temp.pic_id);
	return NULL;
}

static void* threadAlarm(void *arg)
{
	CapData cap_data_temp;
	memcpy(&cap_data_temp,arg,sizeof(CapData));

	int i;
	char file_path[64] = {0};
	char url[256] = {0};
	static ReportAlarmData alarm_data;
	alarm_data.type = ALARM_TYPE_PEOPLES;
	alarm_data.picture_id = cap_data_temp.pic_id;
	alarm_data.has_people = 0;
	for (i=0; i<cap_data_temp.count; i++) {
		sprintf(file_path,"%s%s_%d.jpg",FAST_PIC_PATH,cap_data_temp.file_name,i);
#ifdef USE_VIDEO
		rkVideoCapture(file_path);
#endif
		sprintf(url,"%s/%s_%d.jpg",QINIU_URL,cap_data_temp.file_name,i);
		sqlInsertPicUrlNoBack(cap_data_temp.pic_id,url);

		// wait for write file
		usleep(500000);

		char pic_buf_jpg[100 * 1024] = {0};
		unsigned char *pic_buf_yuv = NULL;
		int yuv_len = 0;
		int w,h;
		FILE *fp = fopen(file_path,"rb");
		int leng = fread(pic_buf_jpg,1,sizeof(pic_buf_jpg),fp);
		fclose(fp);
		jpegToYuv420sp((unsigned char *)pic_buf_jpg, leng,&w,&h, &pic_buf_yuv, &yuv_len);
		if (my_video->faceRecognizer(pic_buf_yuv,w,h,&alarm_data.age,&alarm_data.sex) == 0)
			alarm_data.has_people = 1;
		if (pic_buf_yuv)
			free(pic_buf_yuv);
	}
	sqlInsertRecordAlarm(alarm_data.date,
			alarm_data.type,
			alarm_data.has_people,
			alarm_data.age,
			alarm_data.sex,
			alarm_data.picture_id);
	protocol_hardcloud->uploadPic(FAST_PIC_PATH,alarm_data.picture_id);
	protocol_hardcloud->reportAlarm(&alarm_data);
	return NULL;
}
static void* threadFace(void *arg)
{
	CapData cap_data_temp;
	memcpy(&cap_data_temp,arg,sizeof(CapData));

	int i;
	char file_path[64] = {0};
	char url[256] = {0};
	static ReportFaceData face_data;
	
	strcpy(face_data.date,cap_data_temp.file_date);
	strcpy(face_data.nick_name,cap_data_temp.nick_name);
	strcpy(face_data.user_id,cap_data_temp.usr_id);
	face_data.picture_id = cap_data_temp.pic_id;

	for (i=0; i<cap_data_temp.count; i++) {
		sprintf(file_path,"%s%s_%d.jpg",FAST_PIC_PATH,cap_data_temp.file_name,i);
#ifdef USE_VIDEO
		rkVideoCapture(file_path);
#endif
		sprintf(url,"%s/%s_%d.jpg",QINIU_URL,cap_data_temp.file_name,i);
		sqlInsertPicUrlNoBack(cap_data_temp.pic_id,url);

		// wait for write file
		usleep(500000);

	}

	sqlInsertRecordFaceNoBack(face_data.date,
			face_data.user_id,
			face_data.nick_name,
			face_data.picture_id);
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
	switch(data_temp->cap_type)
	{
		case CAP_TYPE_FORMMAIN :
		case CAP_TYPE_TALK :
			sqlInsertRecordCapNoBack(cap_data.file_date,cap_data.pic_id);
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
		return;
        avi->WriteVideo(avi,data,size);
    }
}

static void recordStopCallbackFunc(void)
{
    pthread_mutex_lock(&mutex);
    if (avi) 
        avi->DestoryMPEG4(&avi);
    pthread_mutex_unlock(&mutex);
}

#if (defined X86)
static void* threadAviReadVideo(void *arg)
{
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
    int audio_fp = -1;
    avi->InitAudio(avi,2,8000,1024);
	if (my_mixer)
		my_mixer->InitPlayAndRec(my_mixer,&audio_fp,8000,2);
    while (avi) {
        char audio_buff[1024] = {0};
		int real_size = my_mixer->Read(my_mixer,audio_buff,sizeof(audio_buff));
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
	StmData *data_temp = (StmData *)data;
	memset(&cap_data,0,sizeof(CapData));
	getFileName(cap_data.file_name,cap_data.file_date);
	cap_data.pic_id = atoll(cap_data.file_name);
	switch(data_temp->cap_type)
	{
		case CAP_TYPE_FORMMAIN :
		case CAP_TYPE_TALK :
            {
                char file_path[64] = {0};
                char url[256] = {0};
                sqlInsertRecordCapNoBack(cap_data.file_date,cap_data.pic_id);
                sprintf(file_path,"%s%s.avi",FAST_PIC_PATH,cap_data.file_name);
                if (avi == NULL) {
                    avi = Mpeg4_Create(320,240,file_path,WRITE_READ,0);
					if (data_temp->cap_type == CAP_TYPE_FORMMAIN)
						createThread(threadAviReadAudio,NULL);
                }
#ifdef USE_VIDEO
                rkVideoRecordStart(recordVideoCallbackFunc);
                rkVideoRecordSetStopFunc(recordStopCallbackFunc);
#endif
#ifdef X86
				createThread(threadAviReadVideo,NULL);
#endif
                sprintf(url,"http://img.cateye.taichuan.com/%s.avi",cap_data.file_name);
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
#ifdef USE_VIDEO
    rkVideoRecordStop();
#endif
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

};

static int stmHandle(StMachine *This,int result,void *data,void *arg)
{
	if (result) {
		return stm_do[This->getCurRun(This)].proc(data,(MyVideo *)arg);
	} else {
		return stm_do[DO_FAIL].proc(data,(MyVideo *)arg);
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
	if (avi) {
		avi->InitAudio(avi,2,8000,size);
        avi->WriteAudio(avi,data,size);
	}
}

static void videoCallOut(char *user_id)
{
	st_data = (StmData *)stm->initPara(stm,
			        sizeof(StmData));
	strcpy(st_data->usr_id,user_id);
	stm->msgPost(stm,EV_TALK_CALLOUT,st_data);
	talk_peer_dev.call_out_result = -1;
}
static int videoGetCallTime(void)
{
	return talk_peer_dev.call_time;
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
static void showLocalVideo(void)
{
#ifdef USE_VIDEO
	rkVideoDisplayLocal();
#else
	IpcData ipc_data;
	ipc_data.dev_type = IPC_DEV_TYPE_MAIN;
	ipc_data.cmd = IPC_VIDEO_ON;
	if (ipc_main)
		ipc_main->sendData(ipc_main,IPC_CAMMER,&ipc_data,sizeof(ipc_data));
#endif
	faceStart();
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
#else
	IpcData ipc_data;
	ipc_data.dev_type = IPC_DEV_TYPE_MAIN;
	ipc_data.cmd = IPC_VIDEO_OFF;
	if (ipc_main)
		ipc_main->sendData(ipc_main,IPC_CAMMER,&ipc_data,sizeof(ipc_data));
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

static void* threadVideoTimer(void *arg)
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
	my_video->showLocalVideo = showLocalVideo;
	my_video->showPeerVideo = showPeerVideo;
	my_video->hideVideo = hideVideo;
	my_video->faceStart = faceStart;
	my_video->faceStop = faceStop;
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
    my_video->recordWriteCallback = recordWriteCallback;
    my_video->delaySleepTime = delaySleepTime;
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
