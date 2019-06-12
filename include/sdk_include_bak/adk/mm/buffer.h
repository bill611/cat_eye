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

#ifndef ADK_MM_BUFFER_H_
#define ADK_MM_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

#include "adk/base/definition_magic.h"
#include "adk/utils/logger.h"

namespace rk {

class Buffer {
 public:
    Buffer() = delete;
    Buffer(size_t size, void* address, uint32_t phys, int fd)
        : size_(size), address_(address), phys_address_(phys), fd_(fd) {}
    Buffer(size_t size, void* address, uint32_t phys)
        : size_(size), address_(address), phys_address_(phys), fd_(-1) {}
    Buffer(size_t size, void* address)
        : size_(size), address_(address), phys_address_(0), fd_(-1) {}
    Buffer(size_t size)
        : size_(size), address_(nullptr), phys_address_(0), fd_(-1) {}

    ADK_DECLARE_SHARED_PTR(Buffer);

    virtual ~Buffer() {
        release();
    }

    virtual size_t size(void) const {
        return size_;
    }

    virtual void* address(void) {
        return address_;
    }

    virtual void set_address(void* addr) {
        address_ = addr;
    }

    virtual void set_phys_address(const uint32_t phys) {
        phys_address_ = phys;
    }

    virtual const uint32_t phys_address(void) const {
        return phys_address_;
    }

    virtual void set_fd(const int fd) {
        fd_ = fd;
    }

    virtual const int fd(void) const {
        return fd_;
    }

    virtual void release(void) {
        size_ = 0;
        address_ = nullptr;
    }

 protected:
    size_t size_;
    void* address_;
    uint32_t phys_address_;
    int fd_;
};

} // namespace rk

#endif // ADK_MM_BUFFER_H_