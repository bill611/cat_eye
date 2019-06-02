/*
 * Face Recognize Library class definition
 *
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

#ifndef RK_FAEC_RECOGNIZE_LIBRARY_H_
#define RK_FAEC_RECOGNIZE_LIBRARY_H_

#include <rk_fb/rk_fb.h>
#include <face/rk_face_recognize_wrapper.h>
#include <adk/mm/buffer.h>

#include "face_recognize_library.h"
#include "loader.h"

namespace rk {

#define RKFACE_RECOGNIZE_MODEL_SIZE (5 * 1024 * 1024)
#define RKFACE_RECOGNIZE_ION_SIZE (5 * 1024 * 1024)

#define MAX_FACE_FEATURE_SIZE 128

class RkFaceRecognizeLibrary : public FaceRecognizeLibrary, LoaderReady {
 public:
    RkFaceRecognizeLibrary();
    virtual ~RkFaceRecognizeLibrary();

    virtual int FeatureExtract(Image& image, Face& face, FaceFeature& feature) override;
    virtual float FeatureCompare(FaceFeature& src, FaceFeature& dst) override;

    virtual int init(void) override {}
    virtual int deinit(void) override {}

    virtual int Ready(void *adderss, uint32_t phys_address);

 private:
    Buffer::SharedPtr ion_;
    Loader::SharedPtr model_;
};

} // namespace rk

#endif
