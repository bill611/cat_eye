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

#ifndef ADK_MM_CMA_BUFFER_H_
#define ADK_MM_CMA_BUFFER_H_

#include <assert.h>

#include <ion/ion.h>
#include <sys/mman.h>

#include "buffer.h"

#include "adk/base/definition_magic.h"
#include "adk/utils/logger.h"

namespace rk {

class CmaBuffer : public Buffer {
 public:
    CmaBuffer() = delete;

    ADK_DECLARE_SHARED_PTR(CmaBuffer);

    CmaBuffer(int ion_handle, int fd,
              uint32_t phys_address, const size_t size)
        : Buffer(size, nullptr, phys_address, fd) {
        ion_handle_ = ion_handle;
    }

    virtual ~CmaBuffer() {
        if (address_)
            munmap(address_, size_);
        release();
    }

    virtual void release(void) override;

    virtual void* address(void) override {
        if (address_ == nullptr) {
            address_ = mmap(NULL, size_, PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_LOCKED, fd_, 0);

            assert(address_ != nullptr);

            pr_info("CmaBuffer mapped, fd=%d, address=0x%p\n", fd_, address_);
        }

        return address_;
    }

    int ion_handle(void) const {
        return ion_handle_;
    }

 private:
    int ion_handle_;
};

} // namespace rk

#endif // ADK_MM_CMA_BUFFER_H_