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

#ifndef ADK_BASE_LANDMARK_H_
#define ADK_BASE_LANDMARK_H_

#include <vector>

namespace rk {

class LandMark {
 public:
    LandMark() = delete;
    LandMark(float x, float y) : x_(x), y_(y) {}

    virtual ~LandMark() {}

    float x(void) const {
        return x_;
    }
    float y(void) const {
        return y_;
    }

 private:
    float x_;
    float y_;
};

class LandMarkArray {
 public:
    LandMarkArray() {}
    virtual ~LandMarkArray() {}

    bool empty(void) const {
        return array_.empty();
    }

    int size(void) const {
        return array_.size();
    }

    std::vector<LandMark> array(void) const {
        return array_;
    }

    void clear(void) {
        array_.clear();
    }

    LandMark back(void) const {
        return array_.back();
    }

    void push_back(LandMark mark) {
        array_.push_back(mark);
    }

    void pop_back(void) {
        array_.pop_back();
    }

    LandMark front(void) const {
        return array_.front();
    }

 private:
    std::vector<LandMark> array_;
};

} // namespace rk

#endif // ADK_BASE_LANDMARK_H_
