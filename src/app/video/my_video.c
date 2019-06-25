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
#include "ucpaas/ucpaas.h"
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
	EV_FACE_ON,			// 打开人脸识别功能
	EV_FACE_OFF,			// 关闭人脸识别功能
	EV_FACE_OFF_FINISH,	// 关闭人脸识别功能结束
	EV_REGIST_FACE,		// 注册人脸
	EV_TALK_ON,	    	// 开始对讲
	EV_TALK_OFF,	    	// 结束对讲
};

enum {
	ST_IDLE,		// 空闲状态
	ST_FACE,		// 人脸识别开启状态
	ST_TALK,		// 对讲状态
	ST_RECORDING, 	// 录像状态
	ST_CAPTURE, 	// 抓拍状态
};

enum {
	DO_FAIL, 		// 信息发送失败
	DO_FACE_ON, 	// 人脸识别开启
	DO_FACE_OFF, 	// 人脸识别关闭
    DO_FACE_REGIST, // 注册人脸
    DO_TALK_ON, 	// 开启对讲
    DO_TALK_OFF, 	// 关闭对讲
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
	{EV_FACE_ON,		ST_IDLE,	ST_FACE,	DO_FACE_ON},

	{EV_FACE_OFF,		ST_FACE,	ST_IDLE,	DO_FACE_OFF},

	{EV_REGIST_FACE,	ST_FACE,	ST_FACE,	DO_FACE_REGIST},

	{EV_TALK_ON,		ST_IDLE,	ST_TALK,	DO_TALK_ON},
	{EV_TALK_ON,		ST_FACE,	ST_TALK,	DO_FACE_OFF},

	{EV_TALK_OFF,		ST_TALK,	ST_IDLE,	DO_TALK_OFF},

	{EV_FACE_OFF_FINISH,ST_TALK,	ST_TALK,	DO_TALK_ON},

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
    stm->msgPost(stm,EV_FACE_OFF_FINISH,NULL);
}

static int stmDoFaceRegist(void *data)
{
	if (my_face)
		return my_face->regist((MyFaceRegistData *)data);
	else
		return -1;
}

static int stmDoTalkOn(void *data)
{
}

static int stmDoTalkOff(void *data)
{
}

static StmDo stm_do[] =
{
	{DO_FAIL,		stmDoFail},
	{DO_FACE_ON,    stmDoFaceOn},
	{DO_FACE_OFF,   stmDoFaceOff},
	{DO_FACE_REGIST,stmDoFaceRegist},
	{DO_TALK_ON,	stmDoTalkOn},
	{DO_TALK_OFF,	stmDoTalkOff},
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
    stm->msgPost(stm,EV_FACE_ON,NULL);
}

static void capture(int count)
{
	// rkVideoStopCapture();
}

static unsigned char *nal_array[300] = { NULL };
static int i_nal = 0;
#define H264SIZE (50*1024)
static unsigned char data264[H264SIZE] = {0};
int TUCS_extern_capture_init()
{
    FILE *f = NULL;
    int data_len;
    unsigned char *fdata = data264;
    f = fopen("stream_chn0.h264", "rb");

    if ( NULL == f)
    {
        return 0;
    }
    memset(data264, 0x00, H264SIZE);
    memset(nal_array, 0x00, sizeof(nal_array));
    i_nal = 0;
	printf("befor fread\n");
    data_len = fread(fdata, sizeof(unsigned char), H264SIZE, f);
    if (f)
    {
        fclose(f);
    }
    
    printf("Read videw %d\n",data_len);
    int len = 0;
    i_nal = 0;

    unsigned char *p = (unsigned char*)fdata;

    while ((len < data_len - 4) && (i_nal < 30))
    {
        if (p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1)
        {
            nal_array[i_nal++] = p;
            len += 4;
            p += 4;
            continue;
        }
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
        {
            nal_array[i_nal++] = p;
            len += 3;
            p += 3;
            continue;
        }
        p++;
        len++;
    }
    
    return 0;
}

// FILE *fd;
// int count = 0;
    // int fn = 0;
static void encCallbackFunc(void *data,int size)
{
	// if (++count < 1000){
		// fwrite(data,1,size,fd);
		// if (count == 1000) {
			// fflush(fd);
			// fclose(fd);
		// }
	// }
		// if (nal_array[fn] != NULL && nal_array[fn+1] != NULL) {
			// ucsSendVideo(nal_array[fn],nal_array[fn + 1] - nal_array[fn]);
		// }
		// fn++;
		// if (fn > i_nal - 3)
		// {
			// fn = 0;
		// }
		ucsSendVideo(data,size);
}
static void recordStart(int count)
{
	// TUCS_extern_capture_init();
	// count = 0;
	// fd = fopen("480p.h264","wb");
    // fn = 0;
	rkH264EncOn(320,240,encCallbackFunc);
}
static void recordStop(void)
{
	rkH264EncOff();
}

static void transVideoStart(void)
{
	rkH264EncOn(320,240,encCallbackFunc);
}
static void transVideoStop(void)
{
	rkH264EncOff();
}
static void showVideo(void)
{
	rkVideoDisplayOnOff(1);
	// faceStart();
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
    return stm->msgPostSync(stm,EV_REGIST_FACE,&face_data);
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
	my_video->recordStop = recordStop;
	my_video->transVideoStart = transVideoStart;
	my_video->transVideoStop = transVideoStop;
	init();
	stm = stateMachineCreate(ST_IDLE,
			state_table,
			sizeof (state_table) / sizeof ((state_table) [0]),
			0,
			stmHandle);
}
