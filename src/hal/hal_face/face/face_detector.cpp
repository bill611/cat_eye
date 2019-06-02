/*
 * FaceFeature class definition
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

#define TAG "face_service"

#include <assert.h>

#include <adk/utils/assert.h>
#include <adk/utils/logger.h>

#include "face_detector.h"
#include "face_camera_buffer.h"
#include "face_service.h"

using namespace rk;

FaceDetector::FaceDetector(FaceDetectAlgorithm type)
              : StreamPUBase("FaceDetector", true, true)
{
    state_ = FaceDetectRunning;

		printf("[%s],type:%d\n", __func__,type);
    switch (type) {
    case kRockchipFaceDetect:
        face_detect_lib_ = std::make_shared<RkFaceDetectLibrary>();
        break;
    default:
        ASSERT(0);
    }
}

FaceDetector::~FaceDetector()
{
    face_detect_lib_.reset();
}

FaceArray::SharedPtr FaceDetector::CropImage(FaceArray::SharedPtr face_array,
                                             Image::SharedPtr image, int frame_width, int frame_height)
{
    FaceArray::SharedPtr new_faces = std::make_shared<FaceArray>();
    ASSERT(new_faces.get() != nullptr);

    if (!face_array.get() || face_array->empty())
        return new_faces;

    float ds_factor = 1.0;
    uint32_t image_width = image->width() > image->height() ? image->width() : image->height();
    uint32_t image_height = image->width() > image->height() ? image->height() : image->width();

    if (image_width * image_height <= frame_width * frame_height) {
        float ds_factor_w = (float)frame_width / (image_width);
        float ds_factor_h = (float)frame_height / (image_height);
        ds_factor = (ds_factor_w > ds_factor_h) ? ds_factor_h : ds_factor_w;
    }

    for (int i = 0; i < face_array->size(); i++) {
        Face::SharedPtr face = (*face_array)[i];
        int left = face->rect().left(), top = face->rect().top();
        int right = face->rect().right(), bottom = face->rect().bottom();

        LandMarkArray new_landmarks;

        switch (MAIN_APP_PRE_FACE_ROTATE) {
        case 0:
            left = face->rect().left() * ds_factor;
            top = face->rect().top() * ds_factor;
            right = face->rect().right() * ds_factor;
            bottom = face->rect().bottom() * ds_factor;

            for (LandMark landmark : face->landmarks().array()) {
                LandMark new_landmark(landmark.x() * ds_factor,
                                      landmark.y() * ds_factor);
                new_landmarks.push_back(new_landmark);
            }
            break;
        case 90:
            left = face->rect().top() * ds_factor;
            top = (image_height - face->rect().right()) * ds_factor;
            right = face->rect().bottom() * ds_factor;
            bottom = (image_height - face->rect().left()) * ds_factor;

            for (LandMark landmark : face->landmarks().array()) {
                LandMark new_landmark(landmark.y() * ds_factor,
                                      ((float)image_height - landmark.x()) * ds_factor);
                new_landmarks.push_back(new_landmark);
            }

            break;
        case 270:
            left = (image_width - face->rect().bottom()) * ds_factor;
            top = face->rect().left() * ds_factor;
            right = (image_width - face->rect().top()) * ds_factor;
            bottom = face->rect().right() * ds_factor;

            for (LandMark landmark : face->landmarks().array()) {
                LandMark new_landmark(((float)image_width - landmark.y()) * ds_factor,
                                      landmark.x() * ds_factor);
                new_landmarks.push_back(new_landmark);
            }

            break;
        default:
            pr_err("main_app_rotate_angle error.\n");
            ASSERT(0);
        }

        int new_size = (right - left) * (bottom - left);
        Rect new_rect(top, left, bottom, right);

        Face::SharedPtr new_face = std::make_shared<Face>(face->id(), new_size,
                                                          face->sharpness(),
                                                          new_landmarks, new_rect);
        if (new_face)
            new_faces->push_back(new_face);
    }

    return new_faces;
}

bool FaceDetector::processFrame(shared_ptr<BufferBase> inBuf,
                                shared_ptr<BufferBase> outBuf)
{
    if (state_ == FaceDetectRunning) {
        FaceCameraBuffer::SharedPtr buffer =
                       dynamic_pointer_cast<FaceCameraBuffer>(outBuf);
        ASSERT(buffer.get() != nullptr);

        Image::SharedPtr image = buffer->image();
        FaceArray::SharedPtr faces = CropImage(Detect(*image), image,
                                               inBuf->getWidth(), inBuf->getHeight());
        buffer->set_faces(faces);

        for(FaceDetectListener* listener : listener_list_)
            listener->OnFaceDetected(faces);
    } else {
        pr_info("state is %d.\n", state_);
        return false;
    }

    return true;
}

FaceArray::SharedPtr FaceDetector::Detect(Image& image)
{
    FaceArray::SharedPtr array = std::make_shared<FaceArray>();
    ASSERT(array.get() != nullptr);

    face_detect_lib_->FaceDetect(image, *array);
    return array;
}

void FaceDetector::RegisterListener(FaceDetectListener* listener)
{
    listener_list_.push_front(listener);
}
