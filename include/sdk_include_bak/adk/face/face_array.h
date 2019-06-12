/*
 * FaceArray class definition
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

#ifndef ADK_FACE_FACE_ARRAY_H_
#define ADK_FACE_FACE_ARRAY_H_

#include <deque>

#include <adk/base/definition_magic.h>
#include <adk/face/face.h>

namespace rk {

class FaceArray {
 public:
    FaceArray() {}
    virtual ~FaceArray() {}

    ADK_DECLARE_SHARED_PTR(FaceArray);

    int size(void) const {
        return faces_.size();
    }

    void clear(void) {
        while(!faces_.empty())
            faces_.pop_back();
    }

    void pop_back(void) {
        faces_.pop_back();
    }

    void push_back(Face::SharedPtr& value) {
        faces_.push_back(value);
    }

    Face::SharedPtr back(void) const {
        return faces_.back();
    }

    Face::SharedPtr front(void) const {
        return faces_.front();
    }

    void pop_front() {
        faces_.pop_front();
    }

    void push_front(Face::SharedPtr& value) {
        faces_.push_front(value);
    }

    bool empty(void) {
        return faces_.empty();
    }

    Face::SharedPtr operator[](int pos) {
        return faces_[pos];
    }

    void set_image(Image::SharedPtr image) {
        image_ = image;
    }

    Image::SharedPtr image(void) const {
        return image_;
    }

 private:
    std::deque<Face::SharedPtr> faces_;
    Image::SharedPtr image_;
};

} // namespace rk

#endif // ADK_FACE_FACE_ARRAY_H_
