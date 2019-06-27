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

#ifndef ADK_MM_CMA_ALLOCATOR_H_
#define ADK_MM_CMA_ALLOCATOR_H_

#include <assert.h>

#include <ion/ion.h>

#include "allocator.h"

namespace rk {

// CmaAllocator class is singleton.
class CmaAllocator : public Allocator {
 public:
    CmaAllocator(enum ion_heap_type type) : type_(type) {
        ion_client_ = ion_open();
        usage_ = 0;
        assert(ion_client_ != 0);
    }

    virtual Buffer* alloc(const size_t size) override;
    virtual void free(Buffer* buffer) override;

    static CmaAllocator& GetInstance() {
        static CmaAllocator global_cma_allocator;
        return global_cma_allocator;
    }

    virtual ~CmaAllocator() {
        ion_close(ion_client_);
        ion_client_ = 0;
        assert(usage_ == 0);
    }

 private:
    CmaAllocator() {
        ion_client_ = ion_open();
        usage_ = 0;
        type_ = ION_HEAP_TYPE_DMA;
        assert(ion_client_ != 0);
    }

    CmaAllocator(const CmaAllocator &);
    CmaAllocator& operator = (const CmaAllocator &);

    int usage_;
    int ion_client_;
    enum ion_heap_type type_;
};

#define CmaAlloc(size) CmaAllocator::GetInstance().alloc(size)
#define CmaFree(buffer) CmaAllocator::GetInstance().free(buffer)

} // namespace rk

#endif // ADK_MM_CMA_ALLOCATOR_H_