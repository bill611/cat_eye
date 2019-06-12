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

#ifndef MODEL_BASE_H_
#define MODEL_BASE_H_

#ifndef SZ_1K
#define SZ_1K (1024)
#endif

#ifndef SZ_1M
#define SZ_1M  (SZ_1K * SZ_1K)
#endif

using namespace rk;

typedef enum _FlashType {
  kSpiNor = 0,
  kEmmc,
  //kSpiNandFlash
} FlashType;

typedef enum ion_heap_type MemType;
// typedef enum _MemType {
//   kIonTypeCar = 0,   // ION_HEAP_TYPE_CARVEOUT
//   kIonTypeDma,       // ION_HEAP_TYPE_DMA
//   kIonTypeDrm,       // ION_HEAP_TYPE_DRM
// } MemType;

typedef struct _ModelCfg {
  int        stage;
    // If the need_load is false, mean don't need read data from flash.
  bool       need_load;
  const char *model_name;
  FlashType  flash_type;
  const char *dev_name;
  int        mem_off;
  int        flash_off;
  int        size;
} ModelCfg;

typedef struct _BufferCfg {
  MemType   mem_type;
  int       phy_addr;
  int       size;
} BufferCfg;

typedef struct _ModelMsg {
  ModelCfg model_cfg;
  Buffer::SharedPtr buffer;
} ModelMsg;

#endif // MODEL_BASE_H_