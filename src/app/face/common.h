/*
 * Rockchip App
 *
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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <linux/fb.h>
#include <linux/videodev2.h>
#include <stddef.h>
#include <ion/ion.h>
#include "rk_fb/rk_fb.h"

#define ALIGN(value, bits) (((value) + ((bits) - 1)) & (~((bits) - 1)))
#define ALIGN_CUT(value, bits) ((value) & (~((bits) - 1)))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define BUFFER_COUNT 4

#define FB0                         "/dev/fb0"
#define ION_DEVICE                  "/dev/ion"

#define CIF_TYPE_SENSOR 0
#define CIF_TYPE_CVBS   1
#define CIF_TYPE_MIX    2
#define USB_TYPE_YUYV 0
#define USB_TYPE_MJPEG 1
#define USB_TYPE_H264 2
#define VIDEO_TYPE_ISP 3
#define VIDEO_TYPE_CIF 4
#define VIDEO_TYPE_USB 5

#define CAMARE_FREQ_50HZ 1
#define CAMARE_FREQ_60HZ 2

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct v4l2_info {
    struct v4l2_buffer buf;
    struct v4l2_capability cap;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers reqbuf;
    enum v4l2_buf_type type;
};

struct video_param {
    unsigned short width;
    unsigned short height;
    unsigned short fps;
};

struct public_message {
    unsigned int id;
    unsigned int type;
};

#endif
