/*
 * Face Recognise Library class definition
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

#include <fstream>
#include <unistd.h>
#include <sys/mman.h>

#include <adk/utils/assert.h>
#include <adk/mm/cma_allocator.h>

#include "rk_face_recognize_library.h"
#include "nor_loader.h"
#include "emmc_loader.h"

using namespace rk;

#if USE_MODEL_LOAD
#define LOADER_TYPR EmmcLoader
#else
#define LOADER_TYPR NorLoader
#endif

static const char* g_auth_license = "/data/key.txt";

static bool g_rk_face_inited = false;

int RkFaceRecognizeLibrary::Ready(void *adderss, uint32_t phys_address)
{
    if (rkFaceInit(ion_->address(), (void*)ion_->phys_address(),
                   (void*)phys_address, g_auth_license)) {
        pr_err("rkFaceInit failed.\n");
	} else {
        g_rk_face_inited = true;
        printf("rkFaceInit ok.\n");

	}
}

RkFaceRecognizeLibrary::RkFaceRecognizeLibrary( )
{
    ion_ =  std::shared_ptr<Buffer>(CmaAlloc(RKFACE_RECOGNIZE_ION_SIZE));
    ASSERT(ion_.get() != nullptr);

    model_ = std::make_shared<LOADER_TYPR>(dynamic_cast<LoaderReady*>(this));
    ASSERT(model_.get() != nullptr);
}

RkFaceRecognizeLibrary::~RkFaceRecognizeLibrary()
{
    CmaFree(ion_.get());
}

int RkFaceRecognizeLibrary::FeatureExtract(Image& image, Face& face,
                                           FaceFeature& feature)
{
    if (!g_rk_face_inited) return -1;

    RkImage rkImage;
    rkImage.pixels = (char*)image.data().address();
    rkImage.type = PIXEL_NV12;
    rkImage.width = image.width();
    rkImage.height = image.height();

    struct RkFace rkFace;
    // rkFace.name = {0};
    rkFace.id = face.id();
    rkFace.grade = face.sharpness();
    rkFace.is_living = 0;

    rkFace.rect.x = face.rect().left();
    rkFace.rect.y = face.rect().top();
    rkFace.rect.width = (face.rect().right() - face.rect().left());
    rkFace.rect.height = (face.rect().bottom() - face.rect().top());

    LandMarkArray landmarks = face.landmarks();
    for (int i = 0; i < MAX_FACE_LANDMARK_NUM; i++) {
        struct RkPoint* point = &rkFace.landmarks[i];
        point->x = landmarks.array()[i].x();
        point->y = landmarks.array()[i].y();
    }

    float feature_arr[MAX_FACE_FEATURE_SIZE];
    int ret = rkFaceFeature(feature_arr, &rkImage, &rkFace);
    if (ret)
        printf("rkFaceFeature failed, ret = %d\n", ret);
	else 
        printf("rkFaceFeature ok\n", ret);

    for (int i = 0; i < MAX_FACE_FEATURE_SIZE; i++)
        feature.feature().push_back(feature_arr[i]);

    return 0;
}

float RkFaceRecognizeLibrary::FeatureCompare(FaceFeature& src, FaceFeature& dst)
{
    if (!g_rk_face_inited) return -1;

    if (src.size() != dst.size()) {
        pr_warn("FeatureCompare feature error, src_size = %d, dst_size = %d\n", src.size(), dst.size());
        return 0;
    }

    float src_feature[MAX_FACE_FEATURE_SIZE];
    float dst_feature[MAX_FACE_FEATURE_SIZE];

    for (int i = 0; i < MAX_FACE_FEATURE_SIZE; i++) {
        src_feature[i] = src.feature()[i];
        dst_feature[i] = dst.feature()[i];
    }

    return rkFeatureIdentify(src_feature, dst_feature, MAX_FACE_FEATURE_SIZE);
}
