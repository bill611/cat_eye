/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
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

#ifndef _RK_FB_H_
#define _RK_FB_H_

#include <linux/fb.h>

#include "ion/ion.h"

#define FB0 "/dev/fb0"
#define ION_DEVICE "/dev/ion"

#define OUT_DEVICE_LCD 0
#define OUT_DEVICE_HDMI 1
#define OUT_DEVICE_CVBS_PAL 2
#define OUT_DEVICE_CVBS_NTSC 3

#define IMAGE_WIDTH 1280
#define IMAGE_HEIGHT 720

#define HAL_PIXEL_FORMAT_YCbCr_422_SP 0x10 /* NV16 */
#define HAL_PIXEL_FORMAT_YCrCb_420_SP 0x11 /* NV21 */
#define HAL_PIXEL_FORMAT_YCrCb_NV12 0x20   /* YUY2 */
#define HAL_PIXEL_FORMAT_YCrCb_444 0x25    /* yuv444 */

#define HAL_PIXEL_FORMAT_RGBA_8888 1
#define HAL_PIXEL_FORMAT_RGBX_8888 2
#define HAL_PIXEL_FORMAT_RGB_888 3
#define HAL_PIXEL_FORMAT_RGB_565 4
#define HAL_PIXEL_FORMAT_BGRA_8888 5

#define RK_MAX_BUF_NUM 11
#define RK30_MAX_LAYER_SUPPORT 5
#define RK_WIN_MAX_AREA 4

#define RK_FBIOSET_CONFIG_DONE 0x4628
#define RK_FBIOPUT_COLOR_KEY_CFG 0x4626

enum {
    CSC_BT601, // clip range: 16-235
    CSC_BT709,
    CSC_BT2020,
    CSC_BT601F, // full range: 0-255
};

enum {
    FB_FORMAT_BGRA_8888 = 0x1000,
    FB_FORMAT_RGB_565,
};

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

struct rk_fb_area_par {
    u8 data_format; /*layer data fmt*/
    short ion_fd;
    u32 phy_addr;
    short acq_fence_fd;
    u16 x_offset;
    u16 y_offset;
    u16 xpos; /*start point in panel  --->LCDC_WINx_DSP_ST*/
    u16 ypos;
    u16 xsize; /* display window width/height  -->LCDC_WINx_DSP_INFO*/
    u16 ysize;
    u16 xact; /*origin display window size -->LCDC_WINx_ACT_INFO*/
    u16 yact;
    u16 xvir; /*virtual width/height     -->LCDC_WINx_VIR*/
    u16 yvir;
    u8 fbdc_en;
    u8 fbdc_cor_en;
    u8 fbdc_data_format;
    u16 reserved0;
    u32 reserved1;
};

struct rk_fb_win_par {
    u8 win_id;
    u8 z_order; /*win sel layer*/
    u8 alpha_mode;
    u16 g_alpha_val;
    u8 mirror_en;
    struct rk_fb_area_par area_par[RK_WIN_MAX_AREA];
    u32 reserved0;
};

struct rk_fb_wb_cfg {
    u8 data_format;
    short ion_fd;
    u32 phy_addr;
    u16 xsize;
    u16 ysize;
    u8 reserved0;
    u32 reversed1;
};

struct rk_fb_win_cfg_data {
    u8 wait_fs;
    short ret_fence_fd;
    short rel_fence_fd[RK_MAX_BUF_NUM];
    struct rk_fb_win_par win_par[RK30_MAX_LAYER_SUPPORT];
    struct rk_fb_wb_cfg wb_cfg;
};

struct color_key_cfg {
    u32 win0_color_key_cfg;
    u32 win1_color_key_cfg;
    u32 win2_color_key_cfg;
};

struct video_ion {
    int client;
    int width;
    int height;
    void *buffer;
    int fd;
    ion_user_handle_t handle;
    size_t size;
    unsigned long phys; // should the same to video/common.h
};

struct color_key {
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    unsigned char enable;
};

// must be the same to one in app/video/common.h
struct win {
    int width;
    int height;
    int pixel;
    unsigned char format;
    unsigned char *buffer;
    struct video_ion video_ion;
    unsigned char z_order;
    struct color_key key;
    int buf_ion_fd;
    int buf_width;
    int buf_height;
};

struct fb {
    int fd;
    struct fb_var_screeninfo vi;
    struct fb_fix_screeninfo fi;
    struct rk_fb_win_cfg_data win_cfg;
    // struct ion ion;
    struct win win0;
    // struct win win1;
    struct win video_win0;
    struct win video_win1;
    struct win *video_win_disp;
    struct win *video_win_write;
    int screen_state;
    int pixel_format;
    int out_device;
    int ui_y_offset_disp;
    int ui_y_offset_write;
};

#ifdef __cplusplus
extern "C" {
#endif

int rk_fb_get_resolution(int *width, int *height);
int rk_fb_ui_disp_ext(void) __attribute__((deprecated));
int rk_fb_get_out_device(int *w, int *h);
int rk_fb_set_out_device(int device);
int rk_fb_set_lcd_backlight(int level);
int rk_fb_getvideowins(struct win **win_write, struct win **win_disp)
__attribute__((deprecated));
/* display the input winmsg, and exchange previous disp_win for writing */
int rk_fb_video_disp_ex(struct win *winmsg, int w, int h);
/* off: 0, on: 1 */
int rk_fb_get_screen_state(void);
void rk_fb_screen_on(void);
void rk_fb_screen_off(void);

/* return win0 */
struct win *rk_fb_getuiwin(void);
/* return the writing video win */
struct win *rk_fb_getvideowin(void);

/* the same to rk_fb_video_disp_ex */
int rk_fb_video_disp(struct win *win);
int rk_fb_ui_disp(struct win *win) __attribute__((deprecated));

/* fb init with RGB565 or BGRA8888 */
struct fb *rk_fb_init(int pixel_format);
int rk_fb_deinit(void);

/* win - color_key, the part color_key in win will be not displayed */
struct color_key *rk_fb_get_color_key();
int rk_fb_set_color_key(struct color_key color_key);

/* read the the enable state of fb0; disable: 0, enable: 1 */
int rk_fb_get_screen_state_ex(void);
int rk_fb_get_hdmi_connect(void);
int rk_fb_get_cvbsout_connect(void);
int rk_fb_get_cvbsout_enabled(void);
int rk_fb_get_cvbsout_mode(void);
void rk_fb_set_flip(int flip);
int rk_fb_get_flip(void);
int rk_fb_recalc_rotate(int rotate);
void rk_fb_set_yuv_range(int range);

/*
 * set state of capture displaying wins, return the video_win_disp.
 * UI win is only one, no need to be gotten.
 */
void rk_fb_set_capture_disp(struct win **video_win, int _val);
int rk_fb_get_disphold(void);
void rk_fb_set_disphold(int val);
int rk_fb_ui_getoffset(void);
void rk_fb_onlyui(void);

#ifdef __cplusplus
}
#endif

#endif
