/*
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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include <face/rkFaceDetectWrapper.h>
#include <adk/mm/cma_allocator.h>

#include "rk_face_detect_library.h"

using namespace rk;

static const char* g_face_detect_weight = "/etc/dsp/face/librk_face_detect_weight.bin";

RkFaceDetectLibrary::RkFaceDetectLibrary()
{
    model_ =  std::shared_ptr<Buffer>(CmaAlloc(RKFACE_DETECT_MODEL_SIZE));
	printf("[%s] create\n", __func__);
    rkFaceDetectWrapper_initizlize(model_->address(),
                                   (void*)model_->phys_address(), (char*)g_face_detect_weight);
}

RkFaceDetectLibrary::~RkFaceDetectLibrary()
{
	printf("[%s] release\n", __func__);
    rkFaceDetectWrapper_release();
    CmaFree(model_.get());
}

int RkFaceDetectLibrary::FaceDetect(Image& image, FaceArray& array)
{
    rkFaceDetectInfos result;
    memset(&result, 0, sizeof(rkFaceDetectInfo));

// #define PRINT_FACE_TIME
#ifdef PRINT_FACE_TIME
    struct timeval start_time, end_time;
    float cost_time;
    gettimeofday(&start_time, NULL);
#endif

    rkFaceDetectWrapper_detect(image.data().address(),(void*)image.data().phys_address(),
                               image.width(), image.height(), &result);

#ifdef PRINT_FACE_TIME
    gettimeofday(&end_time, NULL);
    cost_time = 1000000*(end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec);
    printf("rkFaceDetectWrapper_detect cost time: %f ms\n", cost_time / 1000);
#endif

    for (int i = 0; i < result.objectNum; i++) {
        rkFaceDetectInfo* info = &result.objects[i];


        int id = info->id;
        int size = (info->width) * (info->height);
        float sharpness = info->combined_score;


		static int flag = 0;
		if (flag == 0) {
			flag = 1;
			char file_name[32];
			memset(file_name,0,sizeof(file_name));
			sprintf(file_name,"face%d_%d_%d",i,image.width(),image.height());
			FILE *img_fd = fopen(file_name,"wb");
			fwrite(image.data().address(),image.width() * image.height()*3/2,1,img_fd);
			fflush(img_fd);
			fclose(img_fd);
		}
        LandMarkArray landmarks;
        for (int j = 0; j < 5; j++) {
            LandMark landmark(info->landmarks[j].x, info->landmarks[j].y);
            landmarks.push_back(landmark);
        }

        Rect rect(info->y, info->x, info->y + info->height, info->x + info->width);
        Face::SharedPtr face = std::make_shared<Face>(id, size, sharpness, landmarks, rect);
        if (face)
            array.push_back(face);
    }

    return 0;
}
