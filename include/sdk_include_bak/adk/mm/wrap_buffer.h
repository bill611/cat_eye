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

#ifndef ADK_MM_WRAP_BUFFER_H_
#define ADK_MM_WRAP_BUFFER_H_

#include "buffer.h"

#include "adk/base/definition_magic.h"

namespace rk {

typedef void(*ReleaseWrapBuffer)(void* data);

class WrapBuffer : public Buffer {
 public:
    WrapBuffer() = delete;
    WrapBuffer(const int fd, void* address,
               const uint32_t phys_address, const size_t size, void* data)
        : Buffer(size, address, phys_address, fd), private_data_(data) {}

    virtual ~WrapBuffer() {
        release();
    }

    ADK_DECLARE_SHARED_PTR(WrapBuffer);

    virtual void release(void) override {
        if (release_)
            release_(private_data_);
    }

    virtual void RegisterReleaseCallback(ReleaseWrapBuffer release) {
        release_ = release;
    }

    const uint32_t phys_address(void) const override {
        return phys_address_;
    }

    void set_private_data(void* data) {
        private_data_ = data;
    }

    void* private_data(void) const {
        return private_data_;
    }

 private:
    void* private_data_;
    ReleaseWrapBuffer release_;
};

} // namespace rk

#endif // ADK_MM_WRAP_BUFFER_H_