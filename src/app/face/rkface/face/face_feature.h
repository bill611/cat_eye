/*
 * FaceFeature class definition
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

#ifndef FACE_FEATURE_H_
#define FACE_FEATURE_H_

#include <vector>

#include <adk/base/definition_magic.h>
#include <adk/base/feature.h>

namespace rk {

class FaceFeature : public Feature {
 public:
    FaceFeature() {};

    virtual ~FaceFeature() {};

    ADK_DECLARE_SHARED_PTR(FaceFeature);

    std::vector<float>& feature(void) {
        return feature_;
    }

    bool empty(void) const {
        return feature_.empty();
    }

    int size(void) const {
        return feature_.size();
    }

    void clear(void) {
        feature_.clear();
    }

    float back(void) {
        return feature_.back();
    }

    void push_back(float feature) {
        feature_.push_back(feature);
    }

    void pop_back(void) {
        feature_.pop_back();
    }

    float front(void) {
        return feature_.front();
    }

 private:
    std::vector<float> feature_;
};

} // namespace rk

#endif // FACE_FEATURE_H_
