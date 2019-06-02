/*
 * Camera factory class definition
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

#ifndef CAMERA_FACTORY_H_
#define CAMERA_FACTORY_H_

#include "isp_camera.h"
#include "cif_camera.h"

namespace rk {

class CameraFactory final {
 public:
    CameraFactory() {}
    virtual ~CameraFactory() {}

    Camera::SharedPtr CreateCamera(struct rk_cams_dev_info* cam_info, const int index) {
        ASSERT(cam_info != nullptr);

        if (cam_info->cam[index]->type == RK_CAM_ATTACHED_TO_ISP) {
            shared_ptr<CamHwItf> dev = getCamHwItf(&cam_info->isp_dev);

            IspCamera* isp_camera = new IspCamera(dev, cam_info->cam[index]->index);
            return Camera::SharedPtr(isp_camera);
        } else if (cam_info->cam[index]->type == RK_CAM_ATTACHED_TO_CIF) {
            int cif_index = ((struct rk_cif_dev_info*)(cam_info->cam[index]->dev))->cif_index;
            shared_ptr<CamHwItf> dev = shared_ptr<CamHwItf>(
                                       new CamCifDevHwItf(&(cam_info->cif_devs.cif_devs[cif_index])));

            CifCamera* cif_camera = new CifCamera(dev, cam_info->cam[index]->index);
            return Camera::SharedPtr(cif_camera);
        } else {
            ASSERT(0);
        }
    }
};

} // namespace rk

#endif // CAMERA_FACTORY_H_