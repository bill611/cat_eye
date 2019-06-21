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
#define USE_MIDEA_MODE 0
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

static pthread_mutex_t mutex_encode;		//队列控制互斥信号
static bool encode_finished = false;

static void* encoderProcessThread(void *arg)
{
	H264Data h264_info;
	H264Encoder *process = (H264Encoder *)arg;
	int frame = 0;
	while (1) {
		process->queue()->get(process->queue(),&h264_info);
#if USE_MIDEA_MODE
        ImageInfo* info = process->image_info();
		rk::Buffer::SharedPtr encoder_src = process->encoder_src();
		rk::Buffer::SharedPtr encoder_dst = process->encoder_dst();
        memcpy(encoder_src->address(), h264_info.data, h264_info.w * h264_info.h * 3 / 2);

        rkmedia::MediaBuffer src_buffer(encoder_src->address(), encoder_src->size(), encoder_src->fd());
        std::shared_ptr<rkmedia::ImageBuffer> src =
                        std::make_shared<rkmedia::ImageBuffer>(src_buffer, *info);

        rkmedia::MediaBuffer dst_buffer(encoder_dst->address(), encoder_dst->size(), encoder_dst->fd());
        std::shared_ptr<rkmedia::ImageBuffer> dst =
                        std::make_shared<rkmedia::ImageBuffer>(dst_buffer, *info);

        src->SetValidSize(h264_info.w * h264_info.h * 3 / 2);
        dst->SetValidSize(h264_info.w * h264_info.h * 3 / 2);

        if (process->encoder()->Process(src, dst, nullptr) != 0) {
            ENCODE_LOG("encoder process failed.\n");
            ASSERT(0);
        }
		size_t out_len = dst->GetValidSize();
		fprintf(stderr, "frame %d encoded, type %s, out %d bytes\n", frame,
				dst->GetUserFlag() & rkmedia::MediaBuffer::kIntra ? "I frame" : "P frame", out_len);
        if (fwrite(dst->GetPtr(), dst->GetValidSize(), 1, process->fd()) == 0) {
            ENCODE_LOG("fwrite failed\n");
            ASSERT(0);
        }
#else
		unsigned char *out_data = NULL;
		int size = 0;
		if (my_h264enc)
			size = my_h264enc->encode(my_h264enc,h264_info.data, &out_data);
        if (fwrite(out_data, 1, size, process->fd()) == 0) {
            ENCODE_LOG("fwrite failed\n");
        }
		if (out_data)
			free(out_data);
#endif
		if (frame++ > 300)
			break;
	}
	encode_finished = true;
	fflush(process->fd());
	fclose(process->fd());
	if (my_h264enc)
		my_h264enc->unInit(my_h264enc);
	ENCODE_LOG("%s(),%d\n", __func__,__LINE__);
	return NULL;
}

bool H264Encoder::init(int width, int height)
{
	ENCODE_LOG("%s(),%d\n", __func__,__LINE__);
	image_info_->pix_fmt = PIX_FMT_NV12;
	image_info_->width = width;
	image_info_->height = height;
	image_info_->vir_width = ALIGN(width, 16);
	image_info_->vir_height = ALIGN(height, 16);

	MediaConfig config;
	VideoConfig &video_cfg = config.vid_cfg;
	ImageConfig &image_cfg = video_cfg.image_cfg;
	image_cfg.image_info = *image_info_;
	image_cfg.qp_init = 24;
	video_cfg.qp_step = 4;
	video_cfg.qp_min = 12;
	video_cfg.qp_max = 48;
	video_cfg.bit_rate = width * height * 7;
	if (video_cfg.bit_rate > 1000000) {
	  video_cfg.bit_rate /= 1000000;
	  video_cfg.bit_rate *= 1000000;
	}
	video_cfg.frame_rate = 30;
	video_cfg.level = 52;
	video_cfg.gop_size = video_cfg.frame_rate;
	video_cfg.profile = 100;
	video_cfg.rc_quality = "best";
	video_cfg.rc_mode = "cbr";

	ENCODE_LOG("%s(),%d\n", __func__,__LINE__);
	return encoder_->InitConfig(config);
}

H264Encoder::H264Encoder(int frame_width, int frame_height)
    : StreamPUBase("H264Encoder", true, true)
{
	ENCODE_LOG("%s(),%d\n", __func__,__LINE__);
    fd_ = nullptr;
    is_working_ = false;
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex_encode, &mutexattr);

	queue_ = queueCreate("decode_process",QUEUE_BLOCK,sizeof(H264Data));

#if USE_MIDEA_MODE
	image_info_ = (ImageInfo*)malloc(sizeof(ImageInfo));
	ASSERT(image_info_ != nullptr);

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
	ASSERT(ret == true);
#endif
	createThread(encoderProcessThread,this);
	ENCODE_LOG("%s(),%d\n", __func__,__LINE__);
}
H264Encoder::H264Encoder()
{
	ENCODE_LOG("%s(),%d\n", __func__,__LINE__);
    fd_ = nullptr;
    is_working_ = false;
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex_encode, &mutexattr);

	queue_ = queueCreate("decode_process",QUEUE_BLOCK,sizeof(H264Data));
	createThread(encoderProcessThread,this);
}

H264Encoder::~H264Encoder()
{
    CmaFree(encoder_dst_.get());
    CmaFree(encoder_src_.get());
    encoder_.reset();
    free(image_info_);
}


int H264Encoder::Start(int width,int height)
{

	if (my_h264enc)
		my_h264enc->init(my_h264enc,width, height);
    void * data = nullptr;
    size_t size = 0;

    if (fd_) {
        fclose(fd_);
        fd_ = nullptr;
    }

	fd_ = fopen("./h264.h264", "wb");
	ASSERT(fd_ != nullptr);

#if USE_MIDEA_MODE
	encoder_->GetExtraData(data, size);
	if (fwrite(data, size, 1, fd_) == 0) {
		ENCODE_LOG("fwrite failed\n");
		ASSERT(0);
	}
#endif
	is_working_ = true;
    // frame_count_ = 0;

    return 0;
}

int H264Encoder::StartYuv(void)
{

    void * data = nullptr;
    size_t size = 0;

    if (fd_) {
        fclose(fd_);
        fd_ = nullptr;
    }

	fd_ = fopen("yuv.nv12", "wb");
	ASSERT(fd_ != nullptr);

	is_working_ = true;
    // frame_count_ = 0;

    return 0;
}
void H264Encoder::Reset(void)
{
	is_working_ = false;
    // frame_count_ = 0;
}

bool H264Encoder::processFrame(std::shared_ptr<BufferBase> inBuf,
                               std::shared_ptr<BufferBase> outBuf)
{

	if (encode_finished == true || is_working_ == false)
		return true;
	H264Data h264_info;
	h264_info.w = inBuf->getWidth();
	h264_info.h = inBuf->getHeight();
	memcpy(h264_info.data,inBuf->getVirtAddr(),inBuf->getDataSize());
	queue_->post(queue_,&h264_info);
    return true;
}
