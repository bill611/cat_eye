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

#include "encoder_process.h"
#include "thread_helper.h"
#include "debug.h"
// #include "common.h"
// #include "adk/utils/logger.h"

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
typedef struct _H264Data {
	int type;
	int w,h;
	char data[10];
}H264Data;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/

static pthread_mutex_t mutex_encode;		//队列控制互斥信号

static void* encoderProcessThread(void *arg)
{
	H264Data h264_info;
	Queue *queue = (Queue *)arg;
	while (1) {
		queue->get(queue,&h264_info);
	}
	return NULL;	
}
// void* EncoderProcessor(void* data)
// {
    // Encoder* process = (Encoder*)data;
    // ASSERT(process != nullptr);

    // while (process->ProcessorStatus() == kThreadRunning) {
        // EncodeRequest* request = process->PopRequest();
        // if (request == nullptr)
            // continue;

        // ImageInfo* info = process->image_info();
        // Buffer::SharedPtr encoder_src = process->encoder_src();
        // Buffer::SharedPtr encoder_dst = process->encoder_dst();

        // memcpy(encoder_src->address(), request->frame, request->width * request->height * 3 / 2);

        // rkmedia::MediaBuffer src_buffer(encoder_src->address(), encoder_src->size(), encoder_src->fd());
        // std::shared_ptr<rkmedia::ImageBuffer> src =
                        // std::make_shared<rkmedia::ImageBuffer>(src_buffer, *info);

        // rkmedia::MediaBuffer dst_buffer(encoder_dst->address(), encoder_dst->size(), encoder_dst->fd());
        // std::shared_ptr<rkmedia::ImageBuffer> dst =
                        // std::make_shared<rkmedia::ImageBuffer>(dst_buffer, *info);

        // src->SetValidSize(request->width * request->height * 3 / 2);
        // dst->SetValidSize(request->width * request->height * 3 / 2);

        // if (process->encoder()->Process(src, dst, nullptr) != 0) {
            // printf("encoder process failed.\n");
            // ASSERT(0);
        // }

        // if (fwrite(dst->GetPtr(), dst->GetValidSize(), 1, process->fd()) == 0) {
            // printf("fwrite failed\n");
            // ASSERT(0);
        // }

        // process->DestroyRequest(request);
    // }

    // return nullptr;
// }

bool H264Encoder::init(int width, int height)
{
    // image_info_->pix_fmt = PIX_FMT_NV12;
    // image_info_->width = width;
    // image_info_->height = height;
    // image_info_->vir_width = ALIGN(width, 16);
    // image_info_->vir_height = ALIGN(height, 16);

    // MediaConfig config;
    // VideoConfig &video_cfg = config.vid_cfg;
    // ImageConfig &image_cfg = video_cfg.image_cfg;
    // image_cfg.image_info = *image_info_;
    // image_cfg.qp_init = 24;
    // video_cfg.qp_step = 4;
    // video_cfg.qp_min = 12;
    // video_cfg.qp_max = 48;
    // video_cfg.bit_rate = width * height * 7;
    // if (video_cfg.bit_rate > 1000000) {
      // video_cfg.bit_rate /= 1000000;
      // video_cfg.bit_rate *= 1000000;
    // }
    // video_cfg.frame_rate = 30;
    // video_cfg.level = 52;
    // video_cfg.gop_size = video_cfg.frame_rate;
    // video_cfg.profile = 100;
    // video_cfg.rc_quality = "best";
    // video_cfg.rc_mode = "cbr";

    // return encoder_->InitConfig(config);
}

H264Encoder::H264Encoder(int frame_width, int frame_height)
    : StreamPUBase("H264Encoder", true, true)
{
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex_encode, &mutexattr);

	queue = queueCreate("decode_process",QUEUE_BLOCK,sizeof(H264Data));
	createThread(encoderProcessThread,queue);

    // image_info_ = (ImageInfo*)malloc(sizeof(ImageInfo));
    // ASSERT(image_info_ != nullptr);

    rkmedia::REFLECTOR(Encoder)::DumpFactories();

    encoder_ = rkmedia::REFLECTOR(Encoder)::Create<rkmedia::VideoEncoder>("rkmpp_h264", nullptr);
    if (encoder_ == nullptr) {
        std::cout << "h264dec init fail!" << std::endl;
        return;
    }

    encoder_src_ = std::shared_ptr<Buffer>(CmaAlloc(frame_width * frame_height * 3 / 2));
    ASSERT(encoder_src_.get() != nullptr);

    encoder_dst_ = std::shared_ptr<Buffer>(CmaAlloc(frame_width * frame_height * 3 / 2));
    ASSERT(encoder_dst_.get() != nullptr);

    bool ret = init(frame_width, frame_height);
    // ASSERT(ret == true);

}
H264Encoder::H264Encoder()
{
    
}

H264Encoder::~H264Encoder()
{
    // encoder_.reset();
}


int H264Encoder::Start(void)
{

    void * data = nullptr;
    size_t size = 0;

    encoder_->GetExtraData(data, size);
    // if (fwrite(data, size, 1, fd_) == 0) {
        // printf("fwrite failed\n");
        // ASSERT(0);
    // }

    // is_working_ = true;
    // frame_count_ = 0;

    return 0;
}

void H264Encoder::Reset(void)
{
    // is_working_ = false;
    // frame_count_ = 0;
}

bool H264Encoder::processFrame(std::shared_ptr<BufferBase> inBuf,
                               std::shared_ptr<BufferBase> outBuf)
{

    return true;
}
