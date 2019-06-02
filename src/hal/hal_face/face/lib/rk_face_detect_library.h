/*
 * Rockchip Face Detect Library class definition
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

#ifndef RK_FACE_DETECT_LIBRARY_H_
#define RK_FACE_DETECT_LIBRARY_H_

#include <rk_fb/rk_fb.h>
#include <adk/mm/buffer.h>
#include <adk/utils/logger.h>

#include "face_detect_library.h"

namespace rk {

#define RKFACE_DETECT_MODEL_SIZE (9 * 1024 * 1024)

class RkFaceDetectLibrary : public FaceDetectLibrary {
 public:
    RkFaceDetectLibrary();
    virtual ~RkFaceDetectLibrary();
    virtual int FaceDetect(Image& image, FaceArray& array) override;

    virtual int init(void) override {}
    virtual int deinit(void) override {}

 private:
    Buffer::SharedPtr model_;
};

} // namespace rk

#endif // RK_FACE_DETECT_LIBRARY_H_