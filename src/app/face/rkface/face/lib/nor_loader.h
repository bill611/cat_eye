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

#ifndef NOR_LOADER_H_
#define NOR_LOADER_H_

#include "loader.h"

namespace rk {

#define NOR_LOADER_SIZE (5 * 1024 * 1024)

class NorLoader : public Loader {
 public:
    NorLoader(LoaderReady* ready) : Loader(ready) {
		printf("nor------------------\n");
        model_ion_alloc(NOR_LOADER_SIZE);
    }

    virtual ~NorLoader() {}

    int model_ion_free(struct video_ion* video_ion)
    {
        int ret = 0;

        if (video_ion->buffer) {
            munmap(video_ion->buffer, video_ion->size);
            video_ion->buffer = NULL;
        }

        if (video_ion->fd >= 0) {
            pr_err("--- ion free, release fd: %d\n", video_ion->fd);
            close(video_ion->fd);
            video_ion->fd = -1;
        }

        if (video_ion->client >= 0) {
            if (video_ion->handle) {
                ret = ion_free(video_ion->client, video_ion->handle);
                if (ret)
                    pr_err("ion_free failed!\n");
                video_ion->handle = 0;
            }

            ion_close(video_ion->client);
            video_ion->client = -1;
        }

        memset(video_ion, 0, sizeof(struct video_ion));
        video_ion->client = -1;
        video_ion->fd = -1;

        return ret;
    }

    int model_ion_alloc(size_t size)
    {
        struct video_ion ion_buffer;
        memset(&ion_buffer, 0, sizeof(ion_buffer));

        ion_buffer.fd = -1;
        ion_buffer.client = -1;

        ion_buffer.client = ion_open();
        ion_buffer.size = size;
        if (ion_buffer.client < 0) {
            pr_err("ion open fail, client = %d", ion_buffer.client);
            return -1;
        }

        int ret = ion_alloc(ion_buffer.client, size, 0,
                            ION_HEAP_CARVEOUT_MASK, 0, &ion_buffer.handle);
        if (ret < 0) {
            model_ion_free(&ion_buffer);
            pr_err("ion_alloc failed!\n");
            return -1;
        }

        ret = ion_share(ion_buffer.client, ion_buffer.handle, &ion_buffer.fd);
        if (ret < 0) {
            model_ion_free(&ion_buffer);
            pr_err("ion_share failed!\n");
            return -1;
        }

        ion_get_phys(ion_buffer.client, ion_buffer.handle, &ion_buffer.phys);
        ion_buffer.buffer = mmap(NULL, size, PROT_READ | PROT_WRITE,
                                 MAP_SHARED | MAP_LOCKED, ion_buffer.fd, 0);
        if (!ion_buffer.buffer) {
            model_ion_free(&ion_buffer);
            pr_err("%s mmap failed!\n", __func__);
            return -1;
        }
        pr_dbg("phy = %#x\n", (unsigned int)ion_buffer.phys);

        // Model loaded at physical address 0x67000000
        pr_dbg("---------- Nor Load Model ------------\n");
        for(LoaderReady* ready : ready_list_)
            ready->Ready(ion_buffer.buffer, ion_buffer.phys);

        return 0;
    }

};

} // namespace rk

#endif // NOR_LOADER_H_
