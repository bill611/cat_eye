/*
 * Copyright (C) 2019 Rockchip Electronics Co., Ltd.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL), available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR INv110
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if USE_H264_ENCODER

#include <rk_rga/rk_rga.h>
#include <adk/mm/cma_allocator.h>
#include <adk/utils/assert.h>

#include "encoder.h"
#include "common.h"
#include "adk/utils/logger.h"

using namespace rk;

static const char* g_file_path = "/data";

void* EncoderProcessor(void* data)
{
    Encoder* process = (Encoder*)data;
    ASSERT(process != nullptr);

    while (process->ProcessorStatus() == kThreadRunning) {
        EncodeRequest* request = process->PopRequest();
        if (request == nullptr)
            continue;

        ImageInfo* info = process->image_info();
        Buffer::SharedPtr encoder_src = process->encoder_src();
        Buffer::SharedPtr encoder_dst = process->encoder_dst();

        memcpy(encoder_src->address(), request->frame, request->width * request->height * 3 / 2);

        rkmedia::MediaBuffer src_buffer(encoder_src->address(), encoder_src->size(), encoder_src->fd());
        std::shared_ptr<rkmedia::ImageBuffer> src =
                        std::make_shared<rkmedia::ImageBuffer>(src_buffer, *info);

        rkmedia::MediaBuffer dst_buffer(encoder_dst->address(), encoder_dst->size(), encoder_dst->fd());
        std::shared_ptr<rkmedia::ImageBuffer> dst =
                        std::make_shared<rkmedia::ImageBuffer>(dst_buffer, *info);

        src->SetValidSize(request->width * request->height * 3 / 2);
        dst->SetValidSize(request->width * request->height * 3 / 2);

        if (process->encoder()->Process(src, dst, nullptr) != 0) {
            printf("encoder process failed.\n");
            ASSERT(0);
        }

        if (fwrite(dst->GetPtr(), dst->GetValidSize(), 1, process->fd()) == 0) {
            printf("fwrite failed\n");
            ASSERT(0);
        }

        process->DestroyRequest(request);
    }

    return nullptr;
}

bool Encoder::init(int width, int height)
{
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
    // video_cfg.rc_quality = "aq_only";
    // video_cfg.rc_mode = "vbr";
    video_cfg.rc_quality = "best";
    video_cfg.rc_mode = "cbr";

    return encoder_->InitConfig(config);
}

Encoder::Encoder(int frame_width, int frame_height)
    : StreamPUBase("Encoder", true, true)
{
    fd_ = nullptr;
    frame_count_ = 0;
    is_working_ = false;

    image_info_ = (ImageInfo*)malloc(sizeof(ImageInfo));
    ASSERT(image_info_ != nullptr);

    rkmedia::REFLECTOR(Encoder)::DumpFactories();

    encoder_ = rkmedia::REFLECTOR(Encoder)::Create<rkmedia::VideoEncoder>("rkmpp_h264", nullptr);
    ASSERT(encoder_ != nullptr);

    encoder_src_ = std::shared_ptr<Buffer>(CmaAlloc(frame_width * frame_height * 3 / 2));
    ASSERT(encoder_src_.get() != nullptr);

    encoder_dst_ = std::shared_ptr<Buffer>(CmaAlloc(frame_width * frame_height * 3 / 2));
    ASSERT(encoder_dst_.get() != nullptr);

    bool ret = init(frame_width, frame_height);
    ASSERT(ret == true);

    processor_ = std::make_shared<Thread>("EncoderProcessor", EncoderProcessor, this);
    ASSERT(processor_ != nullptr);
}

Encoder::~Encoder()
{
    processor_->set_status(kThreadStopping);
    processor_->join();
    processor_.reset();

    CmaFree(encoder_dst_.get());
    CmaFree(encoder_src_.get());
    encoder_.reset();
    free(image_info_);
}

EncodeRequest* Encoder::CreateRequest(void* address, uint32_t width, uint32_t height)
{
    EncodeRequest* request = (EncodeRequest*)malloc(sizeof(EncodeRequest));
    ASSERT(request != nullptr);

    request->frame = malloc(width * height * 3 / 2);
    ASSERT(request->frame != nullptr);

    memcpy(request->frame, address, width * height * 3 / 2);

    request->width = width;
    request->height = height;

    return request;
}

void Encoder::DestroyRequest(EncodeRequest* request)
{
    ASSERT(request != nullptr);
    free(request->frame);
    free(request);
}

void Encoder::PushRequest(EncodeRequest* request)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ASSERT(request != nullptr);
    cache_.push_back(request);
}

EncodeRequest* Encoder::PopRequest(void)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (cache_.empty()) {
        if (cache_.wait_for(lock, 5) == -1)
        return nullptr;
    }

    EncodeRequest* request = cache_.front();
    cache_.pop_front();
    return request;
}

int Encoder::GetFileName(char* file, int size, const char* path, const char* filetype)
{
    time_t now;
    static time_t last;

    static int count;
    struct tm* timenow;
    int year, mon, day, hour, min, sec;

    time(&now);
    timenow = localtime(&now);

    count = (now == last) ? (count + 1) : 0;

    year = timenow->tm_year + 1900;
    mon = timenow->tm_mon + 1;
    day = timenow->tm_mday;
    hour = timenow->tm_hour;
    min = timenow->tm_min;
    sec = timenow->tm_sec;

    snprintf(file, size, "%s/%04d%02d%02d%02d%02d%02d_%02d.%s",
             path, year, mon, day, hour, min, sec, count, filetype);
    last = now;

    return 0;
}

int Encoder::Start(void)
{
    char file[128] = {0};
    GetFileName(file, 128, g_file_path, "h264");

    if (fd_) {
        fclose(fd_);
        fd_ = nullptr;
    }

    fd_ = fopen(file, "a+");
    ASSERT(fd_ != nullptr);

    void * data = nullptr;
    size_t size = 0;

    encoder_->GetExtraData(data, size);
    if (fwrite(data, size, 1, fd_) == 0) {
        printf("fwrite failed\n");
        ASSERT(0);
    }

    is_working_ = true;
    frame_count_ = 0;

    return 0;
}

void Encoder::Reset(void)
{
    is_working_ = false;
    frame_count_ = 0;
}

bool Encoder::processFrame(shared_ptr<BufferBase> inBuf, shared_ptr<BufferBase> outBuf)
{
    if (is_working_) {
        EncodeRequest* request = CreateRequest(inBuf->getVirtAddr(), inBuf->getWidth(), inBuf->getHeight());
        if (request && frame_count_ < ENCODER_FRAMES) {
            PushRequest(request);
            frame_count_++;
        } else
            Reset();

        if (cache_.size() > 0)
            cache_.signal();
    }

    return true;
}

#endif // USE_H264_ENCODER
