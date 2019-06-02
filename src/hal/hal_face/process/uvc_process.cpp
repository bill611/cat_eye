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

#include "uvc_process.h"

using namespace rk;

UvcProcess::UvcProcess(int i) : StreamPUBase("UvcProcess", true, true) {
    id = i;
    uvc_video_id_add(get_uvc_video_id(id));
    if (uvc_encode_init(&uvc_enc))
        abort();
    uvc_enc.video_id = get_uvc_video_id(id);
}
UvcProcess::~UvcProcess() {
    uvc_video_id_exit(get_uvc_video_id(id));
    uvc_encode_exit(&uvc_enc);
}
bool UvcProcess::processFrame(shared_ptr<BufferBase> inBuf,
        shared_ptr<BufferBase> outBuf) {
    bool ret = false;
    int flag = -1;

    if (uvc_enc.video_id < 0)
        return ret;
    if (inBuf.get()) {
#if 1
        if ( (flag = uvc_rga_process(&uvc_enc, inBuf->getWidth(), inBuf->getHeight(),
                        inBuf->getVirtAddr(), inBuf->getFd())) == -1)
            return false;
#else
        flag = uvc_norga_process(&uvc_enc, inBuf->getVirtAddr(), inBuf->getFd());
#endif
        if (flag == 0)
            ret = uvc_encode_process(&uvc_enc);
    }

    return ret;
}
