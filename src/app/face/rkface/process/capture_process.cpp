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
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if USE_FACE_CAPTURE

#include <stdio.h>
#include <iterator>

#include <rk_rga/rk_rga.h>
#include <adk/mm/cma_allocator.h>
#include <adk/utils/assert.h>

#include "capture_process.h"
#include "face/face_camera_buffer.h"
#include "adk/utils/logger.h"

using namespace rk;

static const char* g_file_path = "/data";

WrittenRequest* CaptureProcess::CreateRequest(void* image)
{
    WrittenRequest* request = (WrittenRequest*)malloc(sizeof(WrittenRequest));
    ASSERT(request != nullptr);

    request->image = image;
    return request;
}

void CaptureProcess::DestroyRequest(WrittenRequest* request)
{
    ASSERT(request != nullptr);
    free(request->image);
    free(request);
}

void CaptureProcess::PushRequest(WrittenRequest* request)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ASSERT(request != nullptr);
    written_list_.push_back(request);
}

WrittenRequest* CaptureProcess::PopRequest(void)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (written_list_.empty()) {
        if (wait_for(lock, 60) == false)
        return nullptr;
    }

    WrittenRequest* request = written_list_.front();
    written_list_.pop_front();

    return request;
}

int CaptureProcess::GetSnapshotName(char* file, int size, const char* path, const char* filetype)
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

int CaptureProcess::SaveCapture(const void* buffer, size_t size, const char* path)
{
    char file[128] = {0};

    GetSnapshotName(file, 128, path, "jpg");
    FILE* fd = fopen(file, "w+");
    if (!fd) {
        printf("fopen %s failed\n", file);
        return -1 ;
    }
    if (fwrite(buffer, size, 1, fd) == 0) {
        printf("fwrite failed\n");
        return -1;
    }

    return fclose(fd);
}

void* WrittenThread(void* param)
{
    CaptureProcess* process = (CaptureProcess*)param;
    ASSERT(process != nullptr);

    while (process->is_working()) {
        WrittenRequest* request = process->PopRequest();
        if (request == nullptr)
            continue;

        ImageInfo* info = process->image_info();
        Buffer::SharedPtr encoder_src = process->encoder_src();
        Buffer::SharedPtr encoder_dst = process->encoder_dst();

        memcpy(encoder_src->address(), request->image, CAPTURE_WIDTH * CAPTURE_HEIGHT * 3 / 2);

        rkmedia::MediaBuffer src_buffer(encoder_src->address(), encoder_src->size(), encoder_src->fd());
        std::shared_ptr<rkmedia::ImageBuffer> src =
                        std::make_shared<rkmedia::ImageBuffer>(src_buffer, *info);

        rkmedia::MediaBuffer dst_buffer(encoder_dst->address(), encoder_dst->size(), encoder_dst->fd());
        std::shared_ptr<rkmedia::ImageBuffer> dst =
                        std::make_shared<rkmedia::ImageBuffer>(dst_buffer, *info);

        src->SetValidSize(CAPTURE_WIDTH * CAPTURE_HEIGHT * 3 / 2);
        dst->SetValidSize(CAPTURE_WIDTH * CAPTURE_HEIGHT * 3 / 2);

        if (process->encoder()->Process(src, dst, nullptr) != 0) {
            printf("encoder process failed.\n");
            ASSERT(0);
        }

        process->SaveCapture(dst->GetPtr(), dst->GetValidSize(), g_file_path);
        process->DestroyRequest(request);
    }
    return nullptr;
}

void CaptureProcess::StartWorker(void)
{
    is_working_ = true;
    int ret = pthread_create(&worker_, nullptr, WrittenThread, (void*)this);
    ASSERT(ret == 0);
}

void CaptureProcess::StopWorker(void)
{
    is_working_ = false;
    pthread_join(worker_, nullptr);
    worker_ = 0;
}

CaptureProcess::CaptureProcess()
    : StreamPUBase("CaptureProcess", true, true)
{
    rga_fd_ = rk_rga_open();
    ASSERT(rga_fd_ > 0);

    rga_dst_ = std::shared_ptr<Buffer>(CmaAlloc(CAPTURE_WIDTH * CAPTURE_HEIGHT * 3 / 2));
    ASSERT(rga_dst_.get() != nullptr);

    encoder_src_ =  std::shared_ptr<Buffer>(CmaAlloc(CAPTURE_WIDTH * CAPTURE_HEIGHT * 3 / 2));
    ASSERT(encoder_src_.get() != nullptr);

    encoder_dst_ =  std::shared_ptr<Buffer>(CmaAlloc(CAPTURE_WIDTH * CAPTURE_HEIGHT * 3 / 2));
    ASSERT(encoder_dst_.get() != nullptr);

    image_info_ = (ImageInfo*)malloc(sizeof(ImageInfo));
    ASSERT(image_info_ != nullptr);

    image_info_->pix_fmt = PIX_FMT_NV12;
    image_info_->width = CAPTURE_WIDTH;
    image_info_->height = CAPTURE_HEIGHT;
    image_info_->vir_width = CAPTURE_WIDTH;
    image_info_->vir_height = CAPTURE_HEIGHT;

    MediaConfig config;
    config.img_cfg.image_info = *image_info_;
    config.img_cfg.qp_init = 10;

    rkmedia::REFLECTOR(Encoder)::DumpFactories();

    encoder_ = rkmedia::REFLECTOR(Encoder)::Create<rkmedia::VideoEncoder>("rkmpp_jpeg", nullptr);
    ASSERT(encoder_ != nullptr);

    bool ret = encoder_->InitConfig(config);
    ASSERT(ret == true);

    StartWorker();
}

CaptureProcess::~CaptureProcess()
{
    StopWorker();
    free(image_info_);
    CmaFree(encoder_dst_.get());
    CmaFree(encoder_src_.get());
    CmaFree(rga_dst_.get());
    rk_rga_close(rga_fd_);
}

Rect CaptureProcess::CroppedRect(Rect rect, uint32_t frame_width, uint32_t frame_height)
{
    int left = 0, top = 0, right = 0, bottom = 0;
    int src_width = rect.right() - rect.left();
    int src_height = rect.bottom() - rect.top();

    left = rect.left() - src_width / 4;
    left = left > 0 ? left : 0;

    top = rect.top() - src_height / 4;
    top = top > 0 ? top : 0;

    src_width = 2 * src_width;
    src_width = (src_width + 0xf) & ~0xf;
    src_width = src_width > frame_width ? frame_width : src_width;

    src_height = 2 * src_height;
    src_height = (src_height + 0xf) & ~0xf;
    src_height = src_height > frame_height ? frame_height : src_height;

    left = (left + src_width) > frame_width ? (frame_width - src_width) : left;
    top = (top + src_height) > frame_height ? (frame_height - src_height) : top;

    left = left & ~0xf;
    top = top & ~0xf;
    right = left + src_width;
    bottom = top + src_height;

    Rect act(top, left, bottom, right);
    return act;
}

CaptureCache* CaptureProcess::CreateCache(Face::SharedPtr face, void* buffer)
{
    CaptureCache* cache = (CaptureCache*)malloc(sizeof(CaptureCache));
    ASSERT(cache != nullptr);

    cache->id = face->id();
    cache->sharpness = face->sharpness();

    cache->image = malloc(CAPTURE_WIDTH * CAPTURE_HEIGHT * 3 / 2);
    memcpy(cache->image, buffer, CAPTURE_WIDTH * CAPTURE_HEIGHT * 3 / 2);

    return cache;
}

void CaptureProcess::DestroyCache(FaceArray::SharedPtr array)
{
    for (std::list<CaptureCache*>::iterator it = cache_list_.begin();
         it != cache_list_.end();) {
        bool cached = false;
        for (int i = 0; i < array->size(); i++) {
            if ((*it)->id == (*array)[i]->id()) {
                cached = true;
                break;
            }
        }

        if (!cached) {
            WrittenRequest* request = CreateRequest((*it)->image);
            if (request)
                PushRequest(request);

            free(*it);
            cache_list_.erase(it++);
        } else {
            it++;
        }
    }
}

bool CaptureProcess::processFrame(shared_ptr<BufferBase> inBuf, shared_ptr<BufferBase> outBuf)
{
    FaceCameraBuffer::SharedPtr buffer =
                    dynamic_pointer_cast<FaceCameraBuffer>(inBuf);
    ASSERT(buffer.get() != nullptr);

    Image::SharedPtr image = buffer->image();
    FaceArray::SharedPtr array = buffer->faces();

    for (int i = 0; i < array->size(); i++) {
        bool cached = false;
        bool replaced = false;

        Face::SharedPtr face = (*array)[i];
        Rect rect = CroppedRect(face->rect(), inBuf->getWidth(), inBuf->getHeight());

        CaptureCache* cache_id = nullptr;
        for (std::list<CaptureCache*>::iterator it = cache_list_.begin();
             it != cache_list_.end(); it++) {
            if ((*it)->id == face->id()) {
                if (face->sharpness() - (*it)->sharpness > REPLACE_THRESHOLD)
                    replaced = true;
                cache_id = (*it);
                cached = true;
                break;
            }
        }

        if (cached && !replaced)
            return true;

        int src_fd, src_w, src_h, dst_fd, dst_w, dst_h;
        int rect_x, rect_y, rect_w, rect_h, src_fmt, dst_fmt;

        src_fd = inBuf->getFd();
        src_w = inBuf->getWidth();
        src_h = inBuf->getHeight();
        src_fmt = RGA_FORMAT_YCBCR_420_SP;

        rect_w = rect.right() - rect.left();
        rect_h = rect.bottom() - rect.top();
        rect_x = rect.left();
        rect_y = rect.top();

        dst_fd = rga_dst_->fd();
        dst_w = CAPTURE_WIDTH;
        dst_h = CAPTURE_HEIGHT;
        dst_fmt = RGA_FORMAT_YCBCR_420_SP;

        int ret = rk_rga_ionfd_to_ionfd_rotate_offset_ext(rga_fd_, src_fd,
                                                          src_w, src_h, src_fmt,
                                                          src_w, src_h, rect_w, rect_h,
                                                          rect_x, rect_y,
                                                          dst_fd, dst_w, dst_h, dst_fmt,
                                                          MAIN_APP_PRE_FACE_ROTATE);
        ASSERT(ret == 0);

        if (!cached) {
            CaptureCache* cache = CreateCache(face, rga_dst_->address());
            if (cache)
                cache_list_.push_back(cache);
        } else if (replaced) {
            memcpy(cache_id->image, rga_dst_->address(), CAPTURE_WIDTH * CAPTURE_HEIGHT * 3 / 2);
            cache_id->sharpness = face->sharpness();
        } else {
            ASSERT(0);
        }
    }

    DestroyCache(array);

    if (written_list_.size() > 0)
        signal();

    return true;
}

#endif // USE_FACE_CAPTURE
