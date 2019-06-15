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

#include "display.h"

#include <stdio.h>
#include <string.h>
#include <rk_fb/rk_fb.h>
#include <adk/utils/assert.h>

#include "ascii_font.h"

void display_clean_screen(void)
{
    struct win * ui_win;
    struct color_key color_key;
    unsigned short rgb565_data;
    unsigned short *ui_buff;
    int i;
    int w, h;

    ui_win = rk_fb_getuiwin();
    ui_buff = (unsigned short *)ui_win->buffer;

    /* enable and set color key */
    color_key.enable = 1;
    color_key.red = (COLOR_KEY_R & 0x1f) << 3;
    color_key.green = (COLOR_KEY_G & 0x3f) << 2;
    color_key.blue = (COLOR_KEY_B & 0x1f) << 3;
    rk_fb_set_color_key(color_key);

    rk_fb_get_out_device(&w, &h);

    /* set ui win color key */
    rgb565_data = (COLOR_KEY_R & 0x1f) << 11 | ((COLOR_KEY_G & 0x3f) << 5) | (COLOR_KEY_B & 0x1f);
    for (i = 0; i < w * h; i ++) {
        ui_buff[i] = rgb565_data;
    }
}

static int display_draw_character(int width,
                                  int height,
                                  void* dstbuf,
                                  char n,
                                  int left,
                                  int top)
{
    int i, j, l;
    unsigned char y = 16, u = 128, v = 128;  // black
    unsigned char *y_addr = NULL, *uv_addr = NULL;
    unsigned char *start_y = NULL, *start_uv = NULL;

    if (n < ASCII_SHOW_MIN || n > ASCII_SHOW_MAX || left < 0 ||
      (left + FONTX) >= width || top < 0 || (top + FONTY) >= height) {
        printf("error input ascii or position.\n");
        return -1;
    }

    y_addr = (unsigned char*)dstbuf + top * width + left;
    uv_addr = (unsigned char*)dstbuf + width * height + top / 2 * width + left;

    if (*y_addr > 0 && *y_addr < 50)
        y = 160;

    for (j = 0; j < FONTY; j++) {
        for (i = 0; i < FONTX / 8; i++) {
            start_y = y_addr + j * width + i * 8;
            start_uv = uv_addr + j / 2 * width + i * 8;
            for (l = 0; l < 8; l++) {
                if (g_ascii_font[n - ASCII_SHOW_MIN][j][i] >> l & 0x1f) {
                    *(start_y + l) = y;
                    if (j % 2 == 0) {
                        if ((i * 8 + l) % 2 == 0)
                            *(start_uv + l) = u;
                        else
                            *(start_uv + l) = v;
                    }
                }
            }
        }
    }

    return 0;
}

void display_draw_string(const char* str,
                         int size,
                         int left,
                         int top,
                         int width,
                         int height,
                         void* dstbuf)
{
    int i = 0;
    for (i = 0; i < size; i++)
        display_draw_character(width, height, dstbuf, *(str + i), FONTX * i + left, top);
}

static int display_draw_vline(unsigned int width,
                              unsigned int height,
                              void* dstbuf,
                              unsigned int top,
                              unsigned int bottom,
                              unsigned int left,
                              unsigned short color,
                              DisplayType type)
{
    if (top >= height || bottom >= height || left >= width)
        return -1;

    switch (type) {
    case RGB565: {
        unsigned short* ui_buff = (unsigned short *)dstbuf;
        for (int i = top; i < bottom; i++)
            ui_buff[i * width + left] = color;
        break;
    }
    case NV12: {
        char Y = 128, U = 128, V = 128;
        char* y_address = (char *)dstbuf;
        for (int i = top; i < bottom; i++)
            y_address[i * width + left] = Y;

        char* uv_address = (char*)dstbuf + width* height;
        for (int j = top; j < bottom; j++) {
            uv_address[j / 2 * width + left - left % 2] = U;
            uv_address[j / 2 * width + left - left % 2 + 1] = V;
        }
        break;
    }
    default:
        ASSERT(0);
    }
    return 0;
}

static int display_draw_hline(unsigned int width,
                              unsigned int height,
                              void* dstbuf,
                              unsigned int left,
                              unsigned int right,
                              unsigned int top,
                              unsigned short color,
                              DisplayType type)
{
    if (left >= width || right >= width || top >= height)
        return -1;

    switch (type) {
    case RGB565: {
        unsigned short* ui_buff = (unsigned short *)dstbuf;
        for (int i = left; i < right; i++)
            ui_buff[top * width + i] = color;
        break;
    }
    case NV12: {
        char Y = 128, U = 128, V = 128;
        char* y_address = (char *)dstbuf;
        for (int i = left; i < right; i++)
            y_address[top * width + i] = Y;

        char* uv_address = (char*)dstbuf + width* height;
        for (int j = left; j < right; j++) {
            uv_address[top / 2 * width + j - j % 2] = U;
            uv_address[top / 2 * width + j - j % 2 + 1] = V;
        }

        break;
    }
    default:
        ASSERT(0);
    }
    return 0;
}

int display_draw_rect(unsigned int width,
                      unsigned int height,
                      void* dstbuf,
                      unsigned int left,
                      unsigned int top,
                      unsigned int right,
                      unsigned int bottom,
                      unsigned short color,
                      DisplayType type)
{
    if ((left >= width) || (right >= width) ||
        (top >= height) || (bottom >= height))
        return -1;

    display_draw_hline(width, height, dstbuf, left, right, top, color, type);
    display_draw_hline(width, height, dstbuf, left, right, bottom, color, type);
    display_draw_vline(width, height, dstbuf, top, bottom, left, color, type);
    display_draw_vline(width, height, dstbuf, top, bottom, right, color, type);

    return 0;
}
