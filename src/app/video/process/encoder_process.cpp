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

#include "h264_enc_dec/mpi_enc_api.h"
#include "encoder_process.h"
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
using namespace rk;

#define ALIGN(value, bits) (((value) + ((bits) - 1)) & (~((bits) - 1)))
#define ALIGN_CUT(value, bits) ((value) & (~((bits) - 1)))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define IAMGE_MAX_W 1280
#define IAMGE_MAX_H 720
#define IMAGE_MAX_DATA (IAMGE_MAX_W * IAMGE_MAX_H * 3 / 2 )

typedef struct _H264Data {
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

static void* encoderProcessThread(void *arg)
{
	H264Data h264_info;
	H264Encoder *process = (H264Encoder *)arg;
	while (process->start_enc() == true) {
		process->queue()->get(process->queue(),&h264_info);
		unsigned char *out_data = NULL;
		int size = 0;
		if (my_h264enc)
			size = my_h264enc->encode(my_h264enc,h264_info.data, &out_data);
		if (process->encCallback())
			process->encCallback()(out_data,size);
		if (out_data)
			free(out_data);
	}
	if (my_h264enc)
		my_h264enc->unInit(my_h264enc);
	ENCODE_LOG("%s(),%d\n", __func__,__LINE__);
	return NULL;
}

H264Encoder::H264Encoder()
{
	ENCODE_LOG("%s(),%d\n", __func__,__LINE__);
    fd_ = nullptr;

	myH264EncInit();

	queue_ = queueCreate("decode_process",QUEUE_BLOCK,sizeof(H264Data));
}

H264Encoder::~H264Encoder()
{
}


int H264Encoder::startEnc(int width,int height,EncCallbackFunc encCallback)
{

	if (my_h264enc)
		my_h264enc->init(my_h264enc,width, height);
	encCallback_ = encCallback;
	start_enc_ = true;
	last_frame_ = false;
	createThread(encoderProcessThread,this);
    return 0;
}

int H264Encoder::stopEnc(void)
{
	start_enc_ = false;
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
	H264Data h264_info;
	h264_info.w = inBuf->getWidth();
	h264_info.h = inBuf->getHeight();
	memcpy(h264_info.data,inBuf->getVirtAddr(),inBuf->getDataSize());
	queue_->post(queue_,&h264_info);
    return true;
}
