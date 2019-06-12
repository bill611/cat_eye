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

#ifndef ADK_BASE_THREAD_H_
#define ADK_BASE_THREAD_H_

#include <pthread.h>

#include <string>

#include "adk/base/definition_magic.h"

namespace rk {

typedef void* (*pthread_func_t)(void* arg);

enum ThreadStatus {
    kThreadUninited = 1,
    kThreadRunning  = 2,
    kThreadWaiting  = 3,
    kThreadStopping = 4,
};

class Thread {
 public:
    Thread(Thread&) = delete;
    Thread(const Thread&) = delete;
    Thread() : tid_(0), status_(kThreadUninited) {}
    explicit Thread(const char* name, pthread_func_t func, void* arg);
    virtual ~Thread();

    ADK_DECLARE_SHARED_PTR(Thread);

    bool joinable(void) const { return tid_ != 0; }

    void join(void) {
        pthread_join(tid_, nullptr);
        tid_ = 0;
    }

    void detach(void) {
        pthread_detach(tid_);
        tid_ = 0;
    }

    const std::string& name(void) const {
        return name_;
    }

    void set_status(const ThreadStatus status) {
        status_ = status;
    }

    const ThreadStatus status(void) const {
        return status_;
    }

    friend void* thread_func_wrapper(void* data);

 private:
    static const size_t kThreadStackSize;
    pthread_t tid_;
    ThreadStatus status_;
    std::string name_;
    pthread_func_t func_;
    void* arg_;
};

} // namespace rk

#endif // ADK_BASE_THREAD_H_