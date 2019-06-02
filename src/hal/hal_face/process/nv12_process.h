/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
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

#ifndef __NV12_PROCESS_H__
#define __NV12_PROCESS_H__

#include <assert.h>
#include <rk_fb/rk_fb.h>

#include "video.h"

class NV12_MIRROR : public StreamPUBase
{
 public:
    NV12_MIRROR(void) : StreamPUBase("NV12_MIRROR", true, false) {
        rga_fd_ = rk_rga_open();
        assert(rga_fd_ >= 0);
    }
    ~NV12_MIRROR(void) {
        rk_rga_close(rga_fd_);
    }

    bool processFrame(shared_ptr<BufferBase> inBuf, shared_ptr<BufferBase> outBuf)
    {
        int dst_fd, dst_w, dst_h, dst_fmt;
        int src_fd, src_w, src_h, src_fmt, vir_w, vir_h;

        src_fd = inBuf->getFd();
        src_w = inBuf->getWidth();
        src_h = inBuf->getHeight();
        src_fmt = RGA_FORMAT_YCBCR_420_SP;
        vir_w = src_w;
        vir_h = src_h;
        dst_fd = outBuf->getFd();
        dst_w = outBuf->getWidth();
        dst_h = outBuf->getHeight();
        dst_fmt = RGA_FORMAT_YCBCR_420_SP;
        rk_rga_ionfd_to_ionfd_mirror(rga_fd_,
                                     src_fd, src_w, src_h, src_fmt, vir_w, vir_h,
                                     dst_fd, dst_w, dst_h, dst_fmt);
        outBuf->setDataSize(outBuf->getWidth() * outBuf->getHeight() * 3 / 2);

        return true;
    }

 private:
    int rga_fd_;
};

class NV12_FLIP : public StreamPUBase
{
 public:
    NV12_FLIP(void) : StreamPUBase("NV12_FLIP", true, false) {
        rga_fd_ = rk_rga_open();
        assert(rga_fd_ >= 0);
    }
    ~NV12_FLIP() {
        rk_rga_close(rga_fd_);
    }

    bool processFrame(shared_ptr<BufferBase> inBuf, shared_ptr<BufferBase> outBuf)
    {
        int dst_fd, dst_w, dst_h, dst_fmt;
        int src_fd, src_w, src_h, src_fmt, vir_w, vir_h;

        src_fd = inBuf->getFd();
        src_w = inBuf->getWidth();
        src_h = inBuf->getHeight();
        src_fmt = RGA_FORMAT_YCBCR_420_SP;
        vir_w = src_w;
        vir_h = src_h;
        dst_fd = outBuf->getFd();
        dst_w = outBuf->getWidth();
        dst_h = outBuf->getHeight();
        dst_fmt = RGA_FORMAT_YCBCR_420_SP;
        rk_rga_ionfd_to_ionfd_rotate(rga_fd_,
                                     src_fd, src_w, src_h, src_fmt, vir_w, vir_h,
                                     dst_fd, dst_w, dst_h, dst_fmt, 180);
        outBuf->setDataSize(outBuf->getWidth() * outBuf->getHeight() * 3 / 2);

        return true;
    }

 private:
    int rga_fd_;
};

class NV12_ROTATE : public StreamPUBase
{
 public:
    NV12_ROTATE(int rotate) : StreamPUBase("NV12_ROTATE", true, false) {
        rotate_ = rotate;
        rga_fd_ = rk_rga_open();
        assert(rga_fd_ >= 0);
    }

    ~NV12_ROTATE() {
        rk_rga_close(rga_fd_);
    }

    bool processFrame(shared_ptr<BufferBase> inBuf, shared_ptr<BufferBase> outBuf) override
    {
        assert(inBuf->getFd() > 0);
        assert(outBuf->getFd() > 0);

        int src_fd = inBuf->getFd();
        int src_w = inBuf->getWidth();
        int src_h = inBuf->getHeight();
        int src_fmt = RGA_FORMAT_YCBCR_420_SP;

        int src_vir_w = src_w;
        int src_vir_h = src_h;

        int dst_fd = outBuf->getFd();
        int dst_w = src_h;
        int dst_h = src_w;
        int dst_fmt = RGA_FORMAT_YCBCR_420_SP;
        int rotate = rotate_;

        int ret = rk_rga_ionfd_to_ionfd_rotate(rga_fd_,
                                               src_fd, src_w, src_h, src_fmt,
                                               src_vir_w, src_vir_h,
                                               dst_fd, dst_w, dst_h, dst_fmt,
                                               rotate);
        assert(ret == 0);

        return true;
    }

 private:
    int rga_fd_;
    int rotate_;
};

class MP_RGA : public StreamPUBase
{
public:
    MP_RGA(void) : StreamPUBase("MP_RGA", true, false) {
        rga_fd_ = rk_rga_open();
        if (rga_fd_ < 0)
            printf("%s open rga fd failed!\n", __func__);
    }
    ~MP_RGA() {
        rk_rga_close(rga_fd_);
    }

    bool processFrame(shared_ptr<BufferBase> inBuf, shared_ptr<BufferBase> outBuf)
    {
        int src_w, src_h, src_fd, dst_w, dst_h, dst_fd;
        src_w = inBuf->getWidth();
        src_h = inBuf->getHeight();
        src_fd = inBuf->getFd();
        dst_w = outBuf->getWidth();
        dst_h = outBuf->getHeight();
        dst_fd = outBuf->getFd();
        if (rk_rga_ionfd_to_ionfd_scal(rga_fd_,
                                       src_fd, src_w, src_h, RGA_FORMAT_YCBCR_420_SP,
                                       dst_fd, dst_w, dst_h, RGA_FORMAT_YCBCR_420_SP,
                                       0, 0, dst_w, dst_h, src_w, src_h)) {
            printf("%s: %d fail!\n", __func__, __LINE__);
            return false;
        }
        outBuf->setDataSize(dst_w * dst_h * 3 / 2);

        return true;
    }

 private:
    int rga_fd_;
};

#endif
