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

#ifndef MODEL_CFG_H_
#define MODEL_CFG_H_

#include "model_base.h"

/* The size need large model_cfg total size */
BufferCfg buf_cfg = {
  .mem_type = ION_HEAP_TYPE_CARVEOUT,
  .phy_addr = 0x67000000,
  .size     = 16 * SZ_1M,
};

/*
 * If the mode_cfg.need_load is flase, mean don't need
 * read data from flash
 */
ModelCfg model_cfg[] =
{
  {
    .stage       = 0,
    // If the need_load is false, mean don't need read data from flash.
    .need_load   = true,
    .model_name  = "face_detect",
    .flash_type  = kEmmc,
    .dev_name    = "/dev/mmcblk0p6", //UserPart5
    .mem_off     = 0,
    .flash_off   = 0,
    .size        = 16 * SZ_1M,
  },
#if 0
  {
    .stage       = 1,
    .need_load   = true,
    .model_name  = "face_recogni",
    .flash_type  = kEmmc,
    .dev_name    = "/dev/mmcblk0p5", //UserPart4
    .mem_off     = 4 * SZ_1M,
    .flash_off   = 4 * SZ_1M,
    .size        = 12 * SZ_1M,
  },
#endif
};

#endif // MODEL_CFG_H_