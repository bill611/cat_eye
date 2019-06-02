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

#ifndef FAEC_RECOGNIZE_LIBRARY_H_
#define FAEC_RECOGNIZE_LIBRARY_H_

#include <adk/base/definition_magic.h>
#include <adk/base/image.h>
#include <adk/base/library.h>
#include <adk/face/face.h>
#include <adk/utils/assert.h>

#include "face/face_feature.h"

namespace rk {

enum FaceRecognizeAlgorithm {
    kRockchipFaceRecognition = 0,
};

class FaceRecognizeLibrary : public Library {
 public:
    FaceRecognizeLibrary() {};
    virtual ~FaceRecognizeLibrary() {};

    ADK_DECLARE_SHARED_PTR(FaceRecognizeLibrary);

    virtual int FeatureExtract(Image& image, Face& face, FaceFeature& feature) = 0;
    virtual float FeatureCompare(FaceFeature& src, FaceFeature& dst) = 0;
};

} // namespace rk

#endif // FAEC_RECOGNIZE_LIBRARY_H_
