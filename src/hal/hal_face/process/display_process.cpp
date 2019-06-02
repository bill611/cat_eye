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

#include <rk_fb/rk_fb.h>

#include "display_process.h"
#include "system/display/display.h"

using namespace rk;

DisplayProcess::DisplayProcess()
     : StreamPUBase("DisplayProcess", true, true)
{
    rga_fd_ = rk_rga_open();
    ASSERT(rga_fd_ >= 0);

    faces_ = std::make_shared<FaceArray>();
    ASSERT(faces_.get() != nullptr);
}

DisplayProcess::~DisplayProcess()
{
    rk_rga_close(rga_fd_);
}

bool DisplayProcess::processFrame(std::shared_ptr<BufferBase> inBuf,
                                  std::shared_ptr<BufferBase> outBuf)
{
    int src_w = inBuf->getWidth();
    int src_h = inBuf->getHeight();
    int src_fd = (int)(inBuf->getFd());

    int vir_w = src_w;
    int vir_h = src_h;
    int src_fmt = RGA_FORMAT_YCBCR_420_SP;

    int disp_width = 0, disp_height = 0;
    struct win* video_win = rk_fb_getvideowin();

    int out_device = rk_fb_get_out_device(&disp_width, &disp_height);

    int dst_fd = video_win->video_ion.fd;
    int dst_fmt = RGA_FORMAT_YCBCR_420_SP;

    int dst_w = disp_width;
    int dst_h = disp_height;
    int rotate_angle = (out_device == OUT_DEVICE_HDMI ? 0 : VIDEO_DISPLAY_ROTATE_ANGLE);

    for (int i = 0; i < faces_->size(); i++) {
        Rect rect = (*faces_)[i]->rect();
        unsigned short rgb565_data;
        unsigned short* dst_buf = (unsigned short *)inBuf->getVirtAddr();
        rgb565_data = (COLOR_RED_R & 0x1f) << 11 | ((COLOR_RED_G & 0x3) << 5) | (COLOR_RED_B & 0x1f);

        display_draw_rect(inBuf->getWidth(), inBuf->getHeight(), dst_buf, rect.left(), rect.top(),
                          rect.right(), rect.bottom(), rgb565_data, NV12);
        display_draw_rect(inBuf->getWidth(), inBuf->getHeight(), dst_buf, rect.left() - 1, rect.top() - 1,
                          rect.right() - 1, rect.bottom() - 1, rgb565_data, NV12);
        display_draw_rect(inBuf->getWidth(), inBuf->getHeight(), dst_buf, rect.left() + 1, rect.top() + 1,
                          rect.right() + 1, rect.bottom() + 1, rgb565_data, NV12);
    }

    int ret = rk_rga_ionfd_to_ionfd_rotate(rga_fd_,
                                           src_fd, src_w, src_h, src_fmt, vir_w, vir_h,
                                           dst_fd, dst_w, dst_h, dst_fmt,
                                           rotate_angle);
    if (ret) {
        pr_err("rk_rga_ionfd_to_ionfd_rotate failed\n");
        return false;
    }

    ASSERT(rk_fb_video_disp(video_win) != -1);

    return true;
}