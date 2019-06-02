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

#ifndef EMMC_LOADER_H_
#define EMMC_LOADER_H_

#if USE_MODEL_LOAD

#include <model_loader/model_loader.h>

#include "loader.h"

namespace rk {

class EmmcLoader : public Loader, ModelListener {
 public:
    EmmcLoader(LoaderReady* ready) : Loader(ready) {
		printf("mmc------------------\n");
        model_loader_ = new ModelLoader();
        model_loader_->SetListener(this);
        model_loader_->Start();
    }
    virtual ~EmmcLoader() {
        delete model_loader_;
    }

    virtual int Notify(void *msg) {
        ModelMsg* model = (ModelMsg *)msg;
        ASSERT(model != nullptr);

        pr_dbg("---------- Emmc Load Model ------------\n");
        for(LoaderReady* ready : ready_list_)
            ready->Ready(model->buffer->address(), model->buffer->phys_address());

        return 0;
    }

 private:
    ModelLoader* model_loader_;
};

} // namespace rk

#endif // USE_MODEL_LOAD

#endif // EMMC_LOADER_H_
