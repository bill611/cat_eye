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

#include "live_detector.h"
#include "face_camera_buffer.h"
#include <adk/utils/logger.h>

using namespace rk;

LiveDetector::LiveDetector(LiveDetectAlgorithm type)
                           : StreamPUBase("LiveDetect", true, true)
{
    switch (type) {
    case kRockchipLiveDetection:
        live_detect_lib_ = std::make_shared<RkLiveDetectLibrary>();
        break;
    default:
        ASSERT(0);
    }
}

LiveDetector::~LiveDetector()
{
    live_detect_lib_.reset();
}

bool LiveDetector::processFrame(shared_ptr<BufferBase> inBuf,
                              shared_ptr<BufferBase> outBuf)
{
    FaceCameraBuffer::SharedPtr buffer =
                    dynamic_pointer_cast<FaceCameraBuffer>(outBuf);
    ASSERT(buffer.get() != nullptr);

    FaceArray::SharedPtr array = buffer->faces();
    if (!array.get() || array->empty())
        return false;

    Face::SharedPtr face = array->back();
    if (face.get() == nullptr)
        return false;

    Rect rect = face->rect();
    Image image(inBuf->getVirtAddr(), (uint32_t)inBuf->getPhyAddr(),
                inBuf->getFd(), inBuf->getWidth(), inBuf->getHeight());

    bool result = live_detect_lib_->LiveDetect2D(image, rect);

    for(LiveDetectListener* listener : listener_list_)
        listener->OnLiveDetected(result);

    return true;
}

void LiveDetector::RegisterListener(LiveDetectListener* listener)
{
    listener_list_.push_front(listener);
}
