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
	int type;
	int w,h;
	char data[IMAGE_MAX_DATA];
}CammerData;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static pthread_mutex_t mutex;		//队列控制互斥信号
static bool get_img_ready; // 是否可以获取图片数据，当在处理图片数据时，不再获取


//#define TEST_WRITE_SP_TO_FILE
static void* faceProcessThread(void *arg)
{
	CammerData camm_info;
	Queue *queue = (Queue *)arg;
	while (1) {
		queue->get(queue,&camm_info);
		if (my_face){
			my_face->recognizer(camm_info.data,camm_info.w,camm_info.h);
		}
		usleep(100000);
		pthread_mutex_lock(&mutex);
		get_img_ready = true;
		pthread_mutex_unlock(&mutex);
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

	cammer_queue = queueCreate("face_process",QUEUE_BLOCK,sizeof(CammerData));
	get_img_ready = true;
	createThread(faceProcessThread,cammer_queue);
}

FaceProcess::~FaceProcess()
{
}

bool FaceProcess::processFrame(std::shared_ptr<BufferBase> inBuf,
                                 std::shared_ptr<BufferBase> outBuf)
{
	if (get_img_ready == false)
		return true;	

	CammerData camm_info;
	camm_info.w = inBuf->getWidth();
	camm_info.h = inBuf->getHeight();
	memcpy(camm_info.data,inBuf->getVirtAddr(),inBuf->getDataSize());
	cammer_queue->post(cammer_queue,&camm_info);
	pthread_mutex_lock(&mutex);
	get_img_ready = false;
	pthread_mutex_unlock(&mutex);

    return true;
}

