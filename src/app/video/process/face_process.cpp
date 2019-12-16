/*
 * =============================================================================
 *
 *       Filename:  face_process.c
 *
 *    Description:  摄像头数据流接口
 *
 *        Version:  1.0
 *        Created:  2019-06-19 11:50:19
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
#include "face_process.h"
#include "thread_helper.h"
#include "my_face.h"
#include "debug.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define IAMGE_MAX_W 1280
#define IAMGE_MAX_H 720
#define IMAGE_MAX_DATA (IAMGE_MAX_W * IAMGE_MAX_H * 3 / 2 )

enum {
	TYPE_GET_FACE,	   // 人脸识别
	TYPE_GET_CAPTURE,  // 抓拍
	TYPE_GET_RECORD,   // 录像
};

typedef struct _CammerData {
	int get_data_end;
	int type;
	int w,h;
	char data[IMAGE_MAX_DATA];
}CammerData;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static CammerData camm_info;
static pthread_mutex_t mutex;		//队列控制互斥信号


//#define TEST_WRITE_SP_TO_FILE
static void* faceProcessThread(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	FaceProcess *process = (FaceProcess *)arg;
	int ret = 0;
	while (process->start_enc() == true) {
		if (camm_info.get_data_end == 0) {
			usleep(10000);
			continue;
		}
		if (my_face){
			ret = my_face->recognizer(camm_info.data,camm_info.w,camm_info.h);
		}
		pthread_mutex_lock(&mutex);
		camm_info.get_data_end = 0;
		pthread_mutex_unlock(&mutex);
		if (ret == 0) {
			sleep(1);
		}
	}
	return NULL;	
}

FaceProcess::FaceProcess()
     : StreamPUBase("FaceProcess", true, true)
{
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);

}

FaceProcess::~FaceProcess()
{
}

bool FaceProcess::processFrame(std::shared_ptr<BufferBase> inBuf,
                                 std::shared_ptr<BufferBase> outBuf)
{

	if (camm_info.get_data_end == 0) {
		camm_info.w = inBuf->getWidth();
		camm_info.h = inBuf->getHeight();
		memcpy(camm_info.data,inBuf->getVirtAddr(),inBuf->getDataSize());
		pthread_mutex_lock(&mutex);
		camm_info.get_data_end = 1;
		pthread_mutex_unlock(&mutex);
	}

    return true;
}

void FaceProcess::faceInit(void)
{
    if (my_face)    
        my_face->init();
	camm_info.get_data_end = 0;
	start_enc_ = true;
	createThread(faceProcessThread,this);
}

void FaceProcess::faceUnInit(void)
{
    if (my_face)    
        my_face->uninit();
	camm_info.get_data_end = 0;
	start_enc_ = false;
}

int FaceProcess::faceRegist(void *data)
{
    MyFaceRegistData *face_data = (MyFaceRegistData *)data;
    if (!my_face)
        return -1;
    return my_face->regist(face_data);
}
