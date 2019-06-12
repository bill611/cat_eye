/*
 * Image class definition
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

#ifndef ADK_BASE_IMAGE_H_
#define ADK_BASE_IMAGE_H_

#include <stdint.h>

#include <adk/mm/buffer.h>
#include <adk/base/definition_magic.h>

namespace rk {

class Image {
 public:
    Image() = delete;

    Image(void* address, uint32_t phys, int fd, uint32_t width, uint32_t height)
        : data_(width * height * 3 / 2, address, phys, fd), width_(width), height_(height) {}

    Image(void* address, uint32_t phys, uint32_t width, uint32_t height)
        : data_(width * height * 3 / 2, address, phys), width_(width), height_(height) {}

    Image(void* address, uint32_t width, uint32_t height)
        : data_(width * height * 3 / 2, address), width_(width), height_(height) {}

    virtual ~Image() {}

    ADK_DECLARE_SHARED_PTR(Image);

    virtual Buffer data(void) const {
        return data_;
    }

    virtual uint32_t width(void) const {
        return width_;
    }

    virtual uint32_t height(void) const {
        return height_;
    }

 private:
    Buffer data_;
    uint32_t width_;
    uint32_t height_;
};

} // namespace rk

#endif // ADK_BASE_IMAGE_H_
