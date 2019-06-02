/*
 * FaceDetector class definition
 *
 * Copyright (C) 2018 Rockchip Electronics Co., Ltd.
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

#ifndef FACE_DETECTOR_H_
#define FACE_DETECTOR_H_

#include <mutex>
#include <condition_variable>

#include <CameraHal/StrmPUBase.h>

#include <adk/base/image.h>
#include <adk/face/face_array.h>

#include "face_evaluator.h"

#include "lib/face_detect_library.h"
#include "lib/rk_face_detect_library.h"

namespace rk {

enum FaceState {
    FaceDetectRunning = 0,
    FaceDetectPaused,
};

class FaceDetectListener {
 public:
    FaceDetectListener() {}
    virtual ~FaceDetectListener() {}

    ADK_DECLARE_SHARED_PTR(FaceDetectListener);

    virtual void OnFaceDetected(FaceArray::SharedPtr face_array) = 0;
};

class FaceDetector : public StreamPUBase {
 public:
    FaceDetector(FaceDetectAlgorithm type);
    virtual ~FaceDetector();

    virtual bool processFrame(shared_ptr<BufferBase> inBuf,
                              shared_ptr<BufferBase> outBuf);
    virtual FaceArray::SharedPtr Detect(rk::Image& image);
    virtual void RegisterListener(FaceDetectListener* listener);

    FaceArray::SharedPtr CropImage(FaceArray::SharedPtr face_array,
                                   Image::SharedPtr image, int frame_width, int frame_height);

    void resume(void) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = FaceDetectRunning;
    }

    void pause(void) {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = FaceDetectPaused;
    }

 private:
    std::mutex mutex_;
    FaceState state_;
    FaceDetectLibrary::SharedPtr face_detect_lib_;
    std::list<FaceDetectListener*> listener_list_;
};

} // namespace rk

#endif // FACE_DETECTOR_H_
