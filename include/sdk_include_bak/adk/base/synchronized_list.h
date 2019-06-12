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

#ifndef ADK_BASE_SYNCHRONIZED_LIST_H_
#define ADK_BASE_SYNCHRONIZED_LIST_H_

#include <condition_variable>
#include <list>
#include <mutex>

namespace rk {
template <class T>
class SynchronizedList {
 public:
    SynchronizedList() {}
    virtual ~SynchronizedList() {}

    const int size(void) const {
        return list_.size();
    }

    void pop_back(void) {
        list_.pop_back();
    }

    void push_back(T& value) {
        list_.push_back(value);
    }

    T& back(void) {
        return list_.back();
    }

    T& front(void) {
        return list_.front();
    }

    void pop_front() {
        list_.pop_front();
    }

    void push_front(T& value) {
        list_.push_front(value);
    }

    void lock(void) {
        mutex_.lock();
    }

    void unlock(void) {
        mutex_.unlock();
    }

    void signal(void) {
        cond_.notify_all();
    }

    void wait(std::unique_lock<std::mutex>& lock) {
        cond_.wait(lock);
    }

    bool empty(void) const {
        return list_.empty();
    }

    int wait_for(std::unique_lock<std::mutex>& lock, uint32_t seconds) {
        if (cond_.wait_for(lock, std::chrono::seconds(seconds)) ==
            std::cv_status::timeout)
            return -1;
        else
            return 0;
    }

    std::mutex& mutex(void) {
        return mutex_;
    }

 private:
    std::list<T> list_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

} // namespace rk

#endif // ADK_BASE_SYNCHRONIZED_LIST_H_