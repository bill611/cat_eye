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

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static CammerData camm_info;
static pthread_mutex_t mutex;		//队列控制互斥信号


static void* faceProcessThread(void *arg)
{
	FaceProcess *process = (FaceProcess *)arg;
	while (process->start_enc() == true) {
		if (camm_info.get_data_end == 0) {
			usleep(10000);
			continue;
		}
        if (process->faceCallback())
            process->faceCallback()(&camm_info,sizeof(camm_info));
		pthread_mutex_lock(&mutex);
		camm_info.get_data_end = 0;
		pthread_mutex_unlock(&mutex);
		usleep(100000);
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

void FaceProcess::faceInit(FaceCallbackFunc faceCallback)
{
    faceCallback_ = faceCallback;
	camm_info.get_data_end = 0;
	start_enc_ = true;
	createThread(faceProcessThread,this);
}

void FaceProcess::faceUnInit(void)
{
	camm_info.get_data_end = 0;
	start_enc_ = false;
}

