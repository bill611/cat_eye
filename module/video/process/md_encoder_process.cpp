/*
 * =============================================================================
 *
 *       Filename:  encoder_process.c
 *
 *    Description:  H264编码
 *
 *        Version:  1.0
 *        Created:  2019-06-19 22:57:16
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
#include <adk/mm/cma_allocator.h>
#include <adk/utils/assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "h264_enc_dec/md_mpi_enc_api.h"
#include "md_encoder_process.h"
#include "thread_helper.h"
#include "libyuv.h"
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
using namespace rk;

#define ALIGN(value, bits) (((value) + ((bits) - 1)) & (~((bits) - 1)))
#define ALIGN_CUT(value, bits) ((value) & (~((bits) - 1)))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define IAMGE_MAX_W 1280
#define IAMGE_MAX_H 720
#define IMAGE_MAX_DATA (IAMGE_MAX_W * IAMGE_MAX_H * 3 / 2 )

typedef struct _H264Data {
	int get_data_end;
	int type;
	int w,h;
	unsigned char data[IMAGE_MAX_DATA];
}H264Data;

#define ENCODE_LOG(...)       \
do {                          \
    printf("\033[1;34;40m");  \
    printf("[h264encoder]");      \
    printf(__VA_ARGS__);      \
    printf("\033[0m");        \
} while (0)
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static H264Data h264_info;
static pthread_mutex_t enc_mutex;
int NV12Scale(unsigned char *psrc_buf, int psrc_w, int psrc_h, unsigned char **pdst_buf, int pdst_w, int pdst_h)
{
	libyuv::FilterModeEnum pfmode = libyuv::kFilterNone;
    unsigned char *i420_buf1 = (unsigned char *)malloc((psrc_w * psrc_h * 3) >> 1);
    unsigned char *i420_buf2 = (unsigned char *)malloc((pdst_w * pdst_h * 3) >> 1);
    *pdst_buf = (unsigned char *)malloc((pdst_w * pdst_h * 3) >> 1);

    /* NV12_1920x1080 -> I420_1920x1080 */
    libyuv::NV12ToI420(&psrc_buf[0],                           psrc_w,
                       &psrc_buf[psrc_w * psrc_h],             psrc_w,
                       &i420_buf1[0],                          psrc_w,
                       &i420_buf1[psrc_w * psrc_h],            psrc_w >> 1,
                       &i420_buf1[(psrc_w * psrc_h * 5) >> 2], psrc_w >> 1,
                       psrc_w, psrc_h);

    /* I420_1920x1080 -> I420_1280x720 */
    libyuv::I420Scale(&i420_buf1[0],                          psrc_w,
                      &i420_buf1[psrc_w * psrc_h],            psrc_w >> 1,
                      &i420_buf1[(psrc_w * psrc_h * 5) >> 2], psrc_w >> 1,
                      psrc_w, psrc_h,
                      &i420_buf2[0],                          pdst_w,
                      &i420_buf2[pdst_w * pdst_h],            pdst_w >> 1,
                      &i420_buf2[(pdst_w * pdst_h * 5) >> 2], pdst_w >> 1,
                      pdst_w, pdst_h,
                      pfmode);

    /* I420_1280x720 -> NV12_1280x720 */
    libyuv::I420ToNV12(&i420_buf2[0],                          pdst_w,
                       &i420_buf2[pdst_w * pdst_h],            pdst_w >> 1,
                       &i420_buf2[(pdst_w * pdst_h * 5) >> 2], pdst_w >> 1,
                       *pdst_buf,                           pdst_w,
                       &(*pdst_buf)[pdst_w * pdst_h],             pdst_w,
                       pdst_w,pdst_h);

    free(i420_buf1);
    free(i420_buf2);

    return 0;
}

static void* encoderProcessThread(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	H264Encoder *process = (H264Encoder *)arg;
	while (process->start_enc() == true) {
		if (h264_info.get_data_end == 0) {
			usleep(10000);
			continue;
		}
		unsigned char *out_data = NULL;
		int size = 0;
        int frame_type = 0;
		unsigned char *nv12_scale_data = NULL;
		NV12Scale(h264_info.data, 1280, 720, &nv12_scale_data, process->getWidth(), process->getHeight());
		if (my_h264enc)
			size = my_h264enc->encode(my_h264enc,nv12_scale_data, &out_data,&frame_type);
		if (process->encCallback())
			process->encCallback()(out_data,size,frame_type);
		if (process->start_record() && process->recordCallback())
			process->recordCallback()(out_data,size,frame_type);
		if (out_data)
			free(out_data);
		if (nv12_scale_data)
			free(nv12_scale_data);
		pthread_mutex_lock(&enc_mutex);
		h264_info.get_data_end = 0;
		pthread_mutex_unlock(&enc_mutex);
	}
	if (my_h264enc)
		my_h264enc->unInit(my_h264enc);
	ENCODE_LOG("%s(),%d\n", __func__,__LINE__);
	return NULL;
}

H264Encoder::H264Encoder()
{
    fd_ = nullptr;

	start_enc_ = false;
    start_record_ = false;

	myH264EncInit();

	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&enc_mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);
}

H264Encoder::~H264Encoder()
{
}


int H264Encoder::startEnc(int width,int height,EncCallbackFunc encCallback)
{
    if (start_enc_ == true)
        return 0;
	width_ = width;
	height_ = height;
	if (my_h264enc)
		my_h264enc->init(my_h264enc,width, height);
	encCallback_ = encCallback;
	start_enc_ = true;
	last_frame_ = false;
	h264_info.get_data_end = 0;
	memset(&h264_info,0,sizeof(h264_info));
	createThread(encoderProcessThread,this);
    return 0;
}

int H264Encoder::stopEnc(void)
{
    if (start_enc_ == false)
        return 0;
	start_enc_ = false;
    recordStop();
}

void H264Encoder::recordStart(EncCallbackFunc recordCallback)
{
    if (start_record_ == true)
        return;
    recordCallback_ = recordCallback;
    recordStopCallback_ = NULL;
    start_record_ = true;
}
void H264Encoder::recordSetStopFunc(RecordStopCallbackFunc recordStopCallback)
{
    recordStopCallback_ = recordStopCallback;
}
void H264Encoder::recordStop(void)
{
    if (start_record_ == false)
        return;
    start_record_ = false;
    if (recordStopCallback_)
        recordStopCallback_();
    recordStopCallback_ = NULL;
    recordCallback_ = NULL;
}

bool H264Encoder::processFrame(std::shared_ptr<BufferBase> inBuf,
                               std::shared_ptr<BufferBase> outBuf)
{

	if (start_enc_ == false) {
		if (last_frame_ == false) {
			last_frame_ = true;
			goto post_frame;
		}
		return true;
	}
post_frame:
	if (h264_info.get_data_end == 0) {
		h264_info.w = inBuf->getWidth();
		h264_info.h = inBuf->getHeight();
		memcpy(h264_info.data,inBuf->getVirtAddr(),inBuf->getDataSize());
		pthread_mutex_lock(&enc_mutex);
		h264_info.get_data_end = 1;
		pthread_mutex_unlock(&enc_mutex);
	}
    return true;
}
