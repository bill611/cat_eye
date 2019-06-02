/*
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

#define TAG "face_service"

#include <adk/utils/assert.h>
#include <adk/utils/logger.h>

#include "isp_camera.h"

using namespace rk;

IspCamera::IspCamera(std::shared_ptr<CamHwItf> dev, int index)
                     : Camera(ISP_CAMERA, dev, index)
{
    mpath_ = dev_->getPath(CamHwItf::MP);
}

IspCamera::~IspCamera(void) {}

void IspCamera::init(frm_info_t& format)
{
    memcpy(&format_, &format, sizeof(frm_info_t));

    HAL_FPS_INFO_t fps_info;
    fps_info.numerator = 1;
    fps_info.denominator = format.fps;

    if (!dev_->setFps(fps_info))
        pr_info("%s: dev set fps is %.2f\n", __func__,
                1.0 * fps_info.denominator / fps_info.numerator);
}

void IspCamera::start(const uint32_t num, std::shared_ptr<FaceCameraBufferAllocator> allocator)
{
    if (mpath()->prepare(format_, num, *allocator, false, 0) == false) {
        pr_err("mpath prepare failed.\n");
        // ASSERT(0);
    }

    if (!mpath()->start()) {
        pr_err("mpath start failed.\n");
        // ASSERT(0);
    }
}

void IspCamera::stop(void)
{
    if (mpath().get()) {
        mpath()->stop();
        mpath()->releaseBuffers();
    }
}
