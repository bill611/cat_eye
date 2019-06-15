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

#ifndef RK_LIVE_DETECT_LIBRARY_H_
#define RK_LIVE_DETECT_LIBRARY_H_

#include <adk/base/image.h>
#include <adk/base/rect.h>

#include "live_detect_library.h"

namespace rk {

class RkLiveDetectLibrary : public LiveDetectLibrary {
 public:
    RkLiveDetectLibrary();
    virtual ~RkLiveDetectLibrary();

    virtual int init(void);
    virtual int deinit(void);

    virtual bool LiveDetect2D(Image& ir_image, Rect& face_rect);
    virtual bool LiveDetect3D(Image& depth_image, Image& ir_image, Rect& face_rect);
};

} // namespace rk

#endif // RK_LIVE_DETECT_LIBRARY_H_
