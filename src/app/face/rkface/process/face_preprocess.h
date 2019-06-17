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

#include <CameraHal/StrmPUBase.h>
#include <rk_rga/rk_rga.h>

#include <adk/base/image.h>
#include <adk/utils/assert.h>
#include <adk/mm/buffer.h>
#include <adk/mm/cma_allocator.h>

#include "face/face_camera_buffer.h"

namespace rk {

class FacePreprocess : public StreamPUBase {
 public:
    FacePreprocess(int width, int height)
        : width_(width), height_(height), StreamPUBase("FacePreprocess", true, true) {
        rga_fd_ = rk_rga_open();
        ASSERT(rga_fd_ >= 0);

        rga_buffer_ = std::shared_ptr<Buffer>(CmaAlloc(width_ * height_ * 3 / 2));
        ASSERT(rga_buffer_.get() != nullptr);
    }

    virtual ~FacePreprocess() {
        CmaFree(rga_buffer_.get());
        rk_rga_close(rga_fd_);
    }

    int32_t process(int32_t& src_fd, int32_t src_w, int32_t src_h, int32_t src_fmt,
                    int32_t& dst_fd, int32_t dst_w, int32_t dst_h, int32_t dst_fmt) {
        return ((MAIN_APP_PRE_FACE_ROTATE > 0) ?
                rk_rga_ionfd_to_ionfd_rotate(rga_fd_,
                                             src_fd, src_w, src_h, src_fmt, src_w, src_h,
                                             dst_fd, dst_w, dst_h, dst_fmt,
                                             MAIN_APP_PRE_FACE_ROTATE) :
                rk_rga_ionfd_to_ionfd_scal(rga_fd_,
                                           src_fd, src_w, src_h, src_fmt,
                                           dst_fd, dst_w, dst_h, dst_fmt,
                                           0, 0, dst_w, dst_h, src_w, src_h));
    }

    virtual bool processFrame(shared_ptr<BufferBase> inBuf,
                              shared_ptr<BufferBase> outBuf) override
    {
        int32_t src_fd = inBuf->getFd();
        int32_t src_w = inBuf->getWidth();
        int32_t src_h = inBuf->getHeight();
        int32_t src_fmt = RGA_FORMAT_YCBCR_420_SP;

        int32_t src_vir_w = src_w;
        int32_t src_vir_h = src_h;

        int32_t dst_fd = rga_buffer_->fd();
        int32_t dst_fmt = RGA_FORMAT_YCBCR_420_SP;
        int32_t ds_width = src_w;
        int32_t ds_height = src_h;

        if (width_ * height_ <= src_w * src_h) {
            float ds_factor_w = width_ / (float)src_w;
            float ds_factor_h = height_ / (float)src_h;
            float ds_factor   = (ds_factor_w > ds_factor_h) ? ds_factor_h : ds_factor_w;

            ds_width = ds_factor * src_w;
            ds_height = ds_factor * src_h;
        }

        int32_t dst_w = (MAIN_APP_PRE_FACE_ROTATE > 0) ? ds_height : ds_width;
        int32_t dst_h = (MAIN_APP_PRE_FACE_ROTATE > 0) ? ds_width : ds_height;

        int32_t ret = process(src_fd, src_w, src_h, src_fmt, dst_fd, dst_w, dst_h, dst_fmt);
        ASSERT(ret == 0);

        Image::SharedPtr image = std::make_shared<Image>(rga_buffer_->address(),
                                                         (uint32_t)rga_buffer_->phys_address(),
                                                         rga_buffer_->fd(),
                                                         dst_w, dst_h);

        FaceCameraBuffer::SharedPtr buffer =
                       dynamic_pointer_cast<FaceCameraBuffer>(outBuf);
        ASSERT(buffer.get() != nullptr);

        buffer->set_image(image);
        return true;
    }

 private:
    int rga_fd_;
    int width_;
    int height_;
    Buffer::SharedPtr rga_buffer_;
};

} // namespace rk
