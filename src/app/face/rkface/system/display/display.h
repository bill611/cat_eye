/*
 * Rockchip App
 *
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: hogan.wang@rock-chips.com
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

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <stdbool.h>

#define MAX_WIN_NUM 6

#define COLOR_KEY_R 0x0
#define COLOR_KEY_G 0x0
#define COLOR_KEY_B 0x1

#define COLOR_RED_R 0x1F
#define COLOR_RED_G 0x11
#define COLOR_RED_B 0x11

#ifdef __cplusplus
extern "C" {
#endif

enum DisplayType {
    RGB565 = 0,
    NV12,
};

void display_draw_string(const char* str, int size, int x_pos, int y_pos, int width, int height, void* dstbuf);
int display_draw_rect(unsigned int width, unsigned int height, void* dstbuf, unsigned int left, unsigned int top,
                      unsigned int right, unsigned int bottom, unsigned short color, DisplayType type);

void display_clean_screen(void);

#ifdef __cplusplus
}
#endif

#endif
