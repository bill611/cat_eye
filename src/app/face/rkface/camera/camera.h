/*
 * Camera class definition
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

#ifndef CAMERA_H_
#define CAMERA_H_

#include <CameraHal/CamHwItf.h>
#include <CameraHal/cam_types.h>
#include <CameraHal/IonCameraBuffer.h>

#include <adk/base/definition_magic.h>
#include <adk/utils/assert.h>

#include "face/face_camera_buffer.h"

namespace rk {

typedef enum {
    ISP_CAMERA,
    CIF_CAMERA,
} CameraType;

class Camera {
 public:
    Camera() = delete;
    Camera(CameraType type, std::shared_ptr<CamHwItf> dev, int index)
        : index_(index), type_(type), dev_(dev) {
        if (dev_->initHw(index) == false)
            ASSERT(0);
    }
    virtual ~Camera() {
        if (dev_.get())
            dev_->deInitHw();
    }

    ADK_DECLARE_SHARED_PTR(Camera);

    virtual CameraType type(void) const {
        return type_;
    }

    virtual int index(void) const {
        return index_;
    }

    virtual frm_info_t& format(void) {
        return format_;
    }

    virtual std::shared_ptr<CamHwItf::PathBase>& mpath(void) {
        return mpath_;
    }

    virtual void init(frm_info_t& format) = 0;
    virtual void start(const uint32_t num, std::shared_ptr<FaceCameraBufferAllocator> allocator) = 0;
    virtual void stop(void) = 0;


 protected:
    frm_info_t format_;
    std::shared_ptr<CamHwItf> dev_;
    std::shared_ptr<CamHwItf::PathBase> mpath_;

 private:
    int index_;
    CameraType type_;
};

} // namespace rk

#endif // CAMERA_H_