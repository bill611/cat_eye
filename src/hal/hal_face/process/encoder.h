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

#ifndef ENCODER_
#define ENCODER_

#if USE_H264_ENCODER

#include <CameraHal/StrmPUBase.h>
#include <rkmedia/encoder.h>
#include <rkmedia/buffer.h>
#include <rkmedia/image.h>
#include <adk/mm/buffer.h>

#include <adk/base/thread.h>
#include <adk/base/synchronized_list.h>

namespace rk {

#define ENCODER_FRAMES 300

typedef struct EncodeRequest {
    void* frame;
    uint32_t width;
    uint32_t height;
} EncodeRequest;

class Encoder : public StreamPUBase {
 public:
    Encoder(int frame_width, int frame_height);
    virtual ~Encoder();

    virtual bool processFrame(shared_ptr<BufferBase> inBuf, shared_ptr<BufferBase> outBuf);

    int Start(void);
    void Reset(void);

    bool init(int width, int height);

    EncodeRequest* CreateRequest(void* address, uint32_t width, uint32_t height);
    void DestroyRequest(EncodeRequest* request);
    void PushRequest(EncodeRequest* request);
    EncodeRequest* PopRequest(void);

    int GetFileName(char* file, int size, const char* path, const char* filetype);

    ThreadStatus ProcessorStatus(void) const {
        if (processor_)
            return processor_->status();

        return kThreadUninited;
    }

    FILE* fd(void) const {
        return fd_;
    }

    ImageInfo* image_info(void) const {
        return image_info_;
    }

    Buffer::SharedPtr encoder_src(void) const {
        return encoder_src_;
    }

    Buffer::SharedPtr encoder_dst(void) const  {
        return encoder_dst_;
    }

    std::shared_ptr<rkmedia::VideoEncoder> encoder(void) const {
        return encoder_;
    }

 private:
    FILE* fd_;
    bool is_working_;
    uint32_t frame_count_;

    std::mutex mutex_;
    ImageInfo* image_info_;

    Buffer::SharedPtr encoder_src_;
    Buffer::SharedPtr encoder_dst_;
    Thread::SharedPtr processor_;

    SynchronizedList<EncodeRequest*> cache_;
    std::shared_ptr<rkmedia::VideoEncoder> encoder_;
};

} // namespace rk

#endif // USE_H264_ENCODER

#endif // ENCODER_