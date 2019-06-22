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
	EVENT_DISPLAY_ON,	// 开视频显示
	EVENT_DISPLAY_OFF,  // 关视频显示
	EVENT_FACE_ON,		// 打开人脸识别功能
};

enum {
	ST_DISPLAY_ON, 	// 视频显示状态
	ST_DISPLAY_OFF, // 视频显示关闭状态
	ST_FACE_ON,		// 人脸识别开启状态
	ST_FACE_OFF,	// 人脸识别关闭状态
	ST_TALK,		// 对讲状态
	ST_RECORDING, 	// 录像状态
	ST_CAPTURE, 	// 抓拍状态
};

enum {
	DO_DISPLAY_ON,	// 打开视频显示
	DO_DISPLAY_OFF, // 关闭视频显示
	DO_FACE_ON, 	// 人脸识别开启
	DO_FACE_OFF, 	// 人脸识别关闭
	DO_FAIL, 		// 信息发送失败
};

typedef struct _StmData {
	int id;
	char str[16];
}StmData;

typedef struct _StmDo {
	int action;
	void (*proc)(void *data);
}StmDo;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
MyVideo *my_video;
static StMachine* stm;
static StateTable state_table[] =
{
	{EVENT_DISPLAY_ON,	ST_DISPLAY_OFF,	ST_DISPLAY_ON,	DO_DISPLAY_ON},

	{EVENT_DISPLAY_OFF,	ST_DISPLAY_ON,	ST_DISPLAY_OFF,	DO_DISPLAY_OFF},

	{EVENT_FACE_ON,		ST_DISPLAY_ON,	ST_DISPLAY_ON,	DO_FACE_ON},

};

static void stmDoFail(void *data)
{
	int msg = *(int *)data;
	printf("%s()%d\n",__func__,msg);
}
static void stmDoDislpayOn(void *data)
{
	rkVideoDisplayOnOff(1);
}
static void stmDoDislpayOff(void *data)
{
	rkVideoDisplayOnOff(0);
}

static StmDo stm_do[] =
{
	{DO_DISPLAY_ON,	stmDoDislpayOn},
	{DO_DISPLAY_OFF,stmDoDislpayOff},
	{DO_FAIL,		stmDoFail},
};

static void stmHandle(StMachine *This,int result,void *data)
{
	if (result) {
		stm_do[This->getCurRun(This)].proc(data);
	} else {
		stm_do[DO_FAIL].proc(data);
	}
}

static void init(void)
{
	jpegIncDecInit();
	myFaceInit();
	rkVideoInit();
}

static void start(void)
{
	if (stm)
		stm->msgPost(stm,EVENT_DISPLAY_ON,NULL);
}
static void stop(void)
{
	if (stm)
		stm->msgPost(stm,EVENT_DISPLAY_OFF,NULL);
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

void myVideoInit(void)
{
	my_video = (MyVideo *) calloc(1,sizeof(MyVideo));
	my_video->start = start;
	my_video->stop = stop;
	my_video->capture = capture;
	my_video->recordStart = recordStart;
	init();
	stm = stateMachineCreate(ST_DISPLAY_OFF,
			state_table,
			sizeof (state_table) / sizeof ((state_table) [0]),
			0,
			stmHandle);
}
