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

#ifndef FACE_EVALUATOR_H_
#define FACE_EVALUATOR_H_

#include <CameraHal/StrmPUBase.h>

#include <adk/base/definition_magic.h>
#include <adk/face/face.h>
#include <adk/face/face_array.h>

#include "face_camera_buffer.h"

#include "lib/rk_face_evaluate_library.h"

namespace rk {

class FaceEvaluator : public StreamPUBase {
 public:
    FaceEvaluator() : StreamPUBase("FaceEvaluator", true, true) {};
    virtual ~FaceEvaluator() {};

    virtual Face::SharedPtr ChooseBestFace(FaceArray::SharedPtr face_array) {
        if (!face_array.get() || face_array->empty())
            return nullptr;

        Face::SharedPtr face = (*face_array)[0];
        for (int i = 0; i < face_array->size(); i++) {
            Face::SharedPtr checking = (*face_array)[i];
            if (evaluator_.CompareFace(*checking, *face) > 0)
                face = checking;
        }

        return (face->sharpness() == 0 ? nullptr : face);
    }

    virtual bool processFrame(shared_ptr<BufferBase> inBuf, shared_ptr<BufferBase> outBuf)  {
        FaceCameraBuffer::SharedPtr buffer =
                        dynamic_pointer_cast<FaceCameraBuffer>(outBuf);
        ASSERT(buffer.get() != nullptr);

        FaceArray::SharedPtr array = buffer->faces();
        if (!array.get() || array->empty())
            return false;

        Face::SharedPtr face = ChooseBestFace(array);
        if (face.get()) {
            buffer->faces()->clear();
            buffer->faces()->push_back(face);
        }

        return (face.get() ? true : false);
    }

 private:
    RkFaceEvaluateLibrary evaluator_;
};

} // namespace rk

#endif // FACE_EVALUATOR_H_
