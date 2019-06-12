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

#ifndef ADK_BASE_DEFINITION_MAGIC_H_
#define ADK_BASE_DEFINITION_MAGIC_H_

#include <memory>

#define ADK_DECLARE_CREATE_METHOD(__TYPE__)         \
  static __TYPE__* create() {                       \
    __TYPE__* pRet = new __TYPE__();                \
    if (pRet && !pRet->init()) {                    \
      return pRet;                                  \
    } else {                                        \
      if (pRet)                                     \
        delete pRet;                                \
      return NULL;                                  \
    }                                               \
  }

#define ADK_DECLARE_SHARED_PTR(__TYPE__)            \
  typedef std::shared_ptr<__TYPE__> SharedPtr


#endif // ADK_BASE_DEFINITION_MAGIC_H_