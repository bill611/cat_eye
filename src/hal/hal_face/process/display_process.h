/*
 * Display process unit class definition
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

#ifndef FACE_DISPLAY_PROCESS_H_
#define FACE_DISPLAY_PROCESS_H_

#include <CameraHal/StrmPUBase.h>
#include <rk_rga/rk_rga.h>

#include <adk/face/face_array.h>
#include <adk/utils/assert.h>

namespace rk {

class DisplayProcess : public StreamPUBase {
 public:
    DisplayProcess();
    virtual ~DisplayProcess();

    bool processFrame(std::shared_ptr<BufferBase> input,
                      std::shared_ptr<BufferBase> output) override;

    void SetFaces(FaceArray::SharedPtr faces) {
        faces_ = faces;
    }
 private:
    int rga_fd_;
    FaceArray::SharedPtr faces_;
};

} // namespace rk

#endif // FACE_DISPLAY_PROCESS_H_
