/*
 * Face class definition
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

#ifndef ADK_FACE_FACE_H_
#define ADK_FACE_FACE_H_

#include <string.h>

#include <adk/base/definition_magic.h>
#include <adk/base/image.h>
#include <adk/base/landmark.h>
#include <adk/base/rect.h>
#include <adk/utils/assert.h>

namespace rk {

class Face {
 public:
    Face() = delete;

    Face(int id, int size, float sharpness, LandMarkArray landmarks, Rect rect)
         :id_(id), size_(size), sharpness_(sharpness), landmarks_(landmarks),
         rect_(rect) {}

    virtual ~Face() {}

    ADK_DECLARE_SHARED_PTR(Face);

    virtual int id(void) const {
        return id_;
    }

    virtual int size(void) const {
        return size_;
    }

    virtual float sharpness(void) const {
        return sharpness_;
    }

    virtual Rect rect(void) const {
        return rect_;
    }

    virtual LandMarkArray landmarks(void) const {
        return landmarks_;
    }

 private:
    int id_;
    int size_;
    float sharpness_;
    LandMarkArray landmarks_;
    Rect rect_;
};

} // namespace rk

#endif // ADK_FACE_FACE_H_
