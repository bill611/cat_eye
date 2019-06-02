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

#define TAG "face_service"

#include <adk/utils/assert.h>
#include <adk/utils/logger.h>

#include "cif_camera.h"

using namespace  rk;

CifCamera::CifCamera(std::shared_ptr<CamHwItf> dev, const int index)
                    : Camera(CIF_CAMERA, dev, index)
{
    mpath_ = dev_->getPath(CamHwItf::MP);
    ASSERT(mpath_ != nullptr);
}

CifCamera::~CifCamera() {}

void CifCamera::init(frm_info_t& format)
{
    memcpy(&format_, &format, sizeof(format));
}

void CifCamera::start(const uint32_t num, std::shared_ptr<FaceCameraBufferAllocator> allocator)
{
    if (mpath()->prepare(format_, num, *allocator, false, 0) == false) {
        pr_err("mpath prepare failed.\n");
        ASSERT(0);
    }

    if (!mpath()->start()) {
        pr_err("mpath start failed.\n");
        ASSERT(0);
    }
}

void CifCamera::stop(void)
{
    if (mpath().get()) {
        mpath()->stop();
        mpath()->releaseBuffers();
    }
}