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

#ifndef CAPTURE_PROCESS_
#define CAPTURE_PROCESS_

#if USE_FACE_CAPTURE

#include <mutex>
#include <condition_variable>

#include <CameraHal/StrmPUBase.h>
#include <rk_fb/rk_fb.h>
#include <rkmedia/encoder.h>
#include <rkmedia/buffer.h>
#include <rkmedia/image.h>

#include <adk/mm/buffer.h>
#include <adk/face/face_array.h>

namespace rk {

#define CAPTURE_WIDTH 160
#define CAPTURE_HEIGHT 160

#define REPLACE_THRESHOLD 0.1

typedef struct CaptureCache {
    uint32_t id;
    float sharpness;
    void* image;
} CaptureCache;

typedef struct WrittenRequest {
    void* image;
} WrittenRequest;

class CaptureProcess : public StreamPUBase {
 public:
    CaptureProcess();
    virtual ~CaptureProcess();
    virtual bool processFrame(shared_ptr<BufferBase> inBuf, shared_ptr<BufferBase> outBuf);

    Rect CroppedRect(Rect rect, uint32_t frame_width, uint32_t frame_height);

    CaptureCache* CreateCache(Face::SharedPtr face, void* buffer);
    void DestroyCache(FaceArray::SharedPtr array);

    WrittenRequest* CreateRequest(void* image);
    void DestroyRequest(WrittenRequest* request);
    void PushRequest(WrittenRequest* request);
    WrittenRequest* PopRequest(void);

    void StartWorker(void);
    void StopWorker(void);

    int GetSnapshotName(char* file, int size, const char* path, const char* filetype);
    int SaveCapture(const void* buffer, size_t size, const char* path);

    bool is_working(void) const {
        return is_working_;
    }

    bool wait_for(std::unique_lock<std::mutex>& lock, uint32_t seconds) {
        if (cond_.wait_for(lock, std::chrono::seconds(seconds)) ==
                std::cv_status::timeout)
            return false;
        else
            return true;
    }

    void signal(void) {
        cond_.notify_all();
    }

    ImageInfo* image_info(void) const {
        return image_info_;
    }

    std::shared_ptr<rkmedia::VideoEncoder> encoder(void) {
        return encoder_;
    }

    Buffer::SharedPtr encoder_src(void) const {
        return encoder_src_;
    }

    Buffer::SharedPtr encoder_dst(void) const {
        return encoder_dst_;
    }

 private:
    int rga_fd_;
    bool is_working_;
    pthread_t worker_;

    std::mutex mutex_;
    std::condition_variable cond_;

    ImageInfo* image_info_;

    Buffer::SharedPtr rga_dst_;
    Buffer::SharedPtr encoder_src_;
    Buffer::SharedPtr encoder_dst_;

    std::shared_ptr<rkmedia::VideoEncoder> encoder_;
    std::list<CaptureCache*> cache_list_;
    std::list<WrittenRequest*> written_list_;
};

} // namespace rk

#endif // USE_FACE_CAPTURE

#endif // CAPTURE_PROCESS_