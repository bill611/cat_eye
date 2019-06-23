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
#include "stateMachine.h"
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
enum {
	EVENT_FACE_ON,		// 打开人脸识别功能
	EVENT_FACE_OFF,		// 关闭人脸识别功能
	EVENT_REGIST_FACE,	// 注册人脸
	EVENT_TALK_ON,	    // 开始对讲
	EVENT_TALK_OFF,	    // 结束对讲
};

enum {
	ST_IDLE,		// 空闲状态
	ST_FACE,		// 人脸识别开启状态
	ST_TALK,		// 对讲状态
	ST_RECORDING, 	// 录像状态
	ST_CAPTURE, 	// 抓拍状态
};

enum {
	DO_FACE_ON, 	// 人脸识别开启
	DO_FACE_OFF, 	// 人脸识别关闭
    DO_FACE_REGIST, // 注册人脸
	DO_FAIL, 		// 信息发送失败
};

typedef struct _StmData {
	int id;
	char str[16];
}StmData;

typedef struct _StmDo {
	int action;
	int (*proc)(void *data);
}StmDo;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
MyVideo *my_video;
static StMachine* stm;
static StateTable state_table[] =
{
	{EVENT_FACE_ON,		ST_IDLE,	ST_FACE,	DO_FACE_ON},
	{EVENT_FACE_OFF,	ST_FACE,	ST_IDLE,	DO_FACE_OFF},
	{EVENT_REGIST_FACE,	ST_FACE,	ST_FACE,	DO_FACE_REGIST},

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
	printf("%s()%d\n",__func__,msg);
}

static int stmDoFaceOn(void *data)
{
    rkVideoFaceOnOff(1);
}

static int stmDoFaceOff(void *data)
{
    rkVideoFaceOnOff(0);
}

static int stmDoFaceRegist(void *data)
{
   if (my_face)
      return my_face->regist((MyFaceRegistData *)data);
   else
       return -1;
}

static StmDo stm_do[] =
{
	{DO_FAIL,		stmDoFail},
	{DO_FACE_ON,    stmDoFaceOn},
	{DO_FACE_OFF,   stmDoFaceOff},
	{DO_FACE_REGIST,stmDoFaceRegist},
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
	rkVideoInit();
}

static void faceStart(void)
{
    stm->msgPost(stm,EVENT_FACE_ON,NULL);
}

static void capture(int count)
{
	rkVideoStopCapture();
}
static void recordStart(int count)
{
	rkVideoStartRecord();
}
static void recordStop(void)
{
	rkVideoStopRecord();
}

static void showVideo(void)
{
	rkVideoDisplayOnOff(1);
    faceStart();
}
static void hideVideo(void)
{
	rkVideoDisplayOnOff(0);
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
    return stm->msgPostSync(stm,EVENT_REGIST_FACE,&face_data);
}
static void faceDelete(char *id)
{
    if (my_face)
        my_face->deleteOne(id);
}

void myVideoInit(void)
{
	my_video = (MyVideo *) calloc(1,sizeof(MyVideo));
	my_video->showVideo = showVideo;
	my_video->hideVideo = hideVideo;
	my_video->faceStart = faceStart;
	my_video->faceRegist = faceRegist;
	my_video->faceDelete = faceDelete;
	my_video->capture = capture;
	my_video->recordStart = recordStart;
	init();
	stm = stateMachineCreate(ST_FACE,
			state_table,
			sizeof (state_table) / sizeof ((state_table) [0]),
			0,
			stmHandle);
}
