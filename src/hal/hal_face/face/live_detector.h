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

#ifndef LIVE_DETECTOR_H_
#define LIVE_DETECTOR_H_

#include <CameraHal/StrmPUBase.h>

#include "lib/live_detect_library.h"
#include "lib/rk_live_detect_library.h"

namespace rk {

class LiveDetectListener {
 public:
    LiveDetectListener() {}
    virtual ~LiveDetectListener() {}

    virtual void OnLiveDetected(bool living) = 0;
};

class LiveDetector : public StreamPUBase {
 public:
    LiveDetector(LiveDetectAlgorithm type);
    virtual ~LiveDetector();

    virtual bool processFrame(shared_ptr<BufferBase> inBuf, shared_ptr<BufferBase> outBuf);
    virtual void RegisterListener(LiveDetectListener* listener);

 private:
    std::list<LiveDetectListener*> listener_list_;
    LiveDetectLibrary::SharedPtr live_detect_lib_;
};

} // namespace rk

#endif // LIVE_DETECTOR_H_