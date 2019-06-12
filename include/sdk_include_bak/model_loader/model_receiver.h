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

#ifndef MODEL_RECEIVER_H_
#define MODEL_RECEIVER_H_

#include <vector>
#include <list>

#include <ion/ion.h>

#include <adk/base/thread.h>
#include <adk/mm/buffer.h>
#include <adk/mm/cma_buffer.h>

#include "model_base.h"
#include "model_listener.h"

namespace rk {

class ModelReceiver {
 public:
  std::list<ModelMsg> msg_list_;
  ModelListener *listener_;
  ModelReceiver();
  virtual ~ModelReceiver();
  Thread::SharedPtr processor_;
  ThreadStatus ProcessorStatus(void) const {
    if (processor_)
        return processor_->status();

    return kThreadUninited;
  }
  void SetListener(ModelListener *listener) {
    listener_ = listener;
  }
 private:
  void Clear();
  int Start();
  int Stop();
};

} // namespace rk

#endif // MODEL_RECEIVER_H_