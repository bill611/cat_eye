#ifndef __RGA_COPYBIT_H__
#define __RGA_COPYBIT_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RGA_BLIT_SYNC 0x5017
#define RGA_BLIT_ASYNC 0x5018
#define RGA_FLUSH 0x5019
#define RGA_GET_RESULT 0x501a
#define RGA_GET_VERSION 0x501b

/* RGA process mode enum */
enum {
  bitblt_mode = 0x0,
  color_palette_mode = 0x1,
  color_fill_mode = 0x2,
  line_point_drawing_mode = 0x3,
  blur_sharp_filter_mode = 0x4,
  pre_scaling_mode = 0x5,
  update_palette_table_mode = 0x6,
  update_patten_buff_mode = 0x7,
};

enum {
  rop_enable_mask = 0x2,
  dither_enable_mask = 0x8,
  fading_enable_mask = 0x10,
  PD_enbale_mask = 0x20,
};

enum {
  yuv2rgb_mode0 = 0x0, /* BT.601 MPEG */
  yuv2rgb_mode1 = 0x1, /* BT.601 JPEG */
  yuv2rgb_mode2 = 0x2, /* BT.709      */
};

/* RGA rotate mode */
enum {
  rotate_mode0 = 0x0, /* no rotate */
  rotate_mode1 = 0x1, /* rotate    */
  rotate_mode2 = 0x2, /* x_mirror  */
  rotate_mode3 = 0x3, /* y_mirror  */
};

enum {
  color_palette_mode0 = 0x0, /* 1K */
  color_palette_mode1 = 0x1, /* 2K */
  color_palette_mode2 = 0x2, /* 4K */
  color_palette_mode3 = 0x3, /* 8K */
};

/*
//          Alpha    Red     Green   Blue
{  4, 32, {{32,24,   8, 0,  16, 8,  24,16 }}, GGL_RGBA },
{  4, 24, {{ 0, 0,   8, 0,  16, 8,  24,16 }}, GGL_RGB  },
{  3, 24, {{ 0, 0,   8, 0,  16, 8,  24,16 }}, GGL_RGB  },
{  4, 32, {{32,24,  24,16,  16, 8,   8, 0 }}, GGL_BGRA },
{  2, 16, {{ 0, 0,  16,11,  11, 5,   5, 0 }}, GGL_RGB  },
{  2, 16, {{ 1, 0,  16,11,  11, 6,   6, 1 }}, GGL_RGBA },
{  2, 16, {{ 4, 0,  16,12,  12, 8,   8, 4 }}, GGL_RGBA },
{  3, 24, {{ 0, 0,  24,16,  16, 8,   8, 0 }}, GGL_BGR  },

*/


struct rga_img_info_t {
  unsigned int yrgb_addr; /* yrgb    mem addr      */
  unsigned int uv_addr;   /* cb/cr   mem addr      */
  unsigned int v_addr;    /* cr      mem addr      */
  unsigned int format;    /*definition by RGA_FORMAT*/
  unsigned short act_w;
  unsigned short act_h;
  unsigned short x_offset;
  unsigned short y_offset;
  unsigned short vir_w;
  unsigned short vir_h;
  unsigned short endian_mode;
  unsigned short alpha_swap;
};

struct mdp_img_act {
  unsigned short w;
  unsigned short h;
  short x_off;
  short y_off;
};

struct RANGE {
  unsigned short min;
  unsigned short max;
};

struct POINT {
  unsigned short x;
  unsigned short y;
};

struct RECT {
  unsigned short xmin;
  /* width - 1 */
  unsigned short xmax;
  unsigned short ymin;
  /* height - 1 */
  unsigned short ymax;
};

struct RGB {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char res;
};

struct MMU {
  unsigned char mmu_en;
  uint32_t base_addr;
  /* [0] mmu enable [1] src_flush [2] dst_flush
     [3] CMD_flush [4~5] page size */
  uint32_t mmu_flag;
};

struct COLOR_FILL {
  short gr_x_a;
  short gr_y_a;
  short gr_x_b;
  short gr_y_b;
  short gr_x_g;
  short gr_y_g;
  short gr_x_r;
  short gr_y_r;
};

struct FADING {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t res;
};

struct line_draw_t {
  /* LineDraw_start_point                */
  struct POINT start_point;
  /* LineDraw_end_point                  */
  struct POINT end_point;
  /* LineDraw_color                      */
  uint32_t color;
  /* (enum) LineDrawing mode sel         */
  uint32_t flag;
  /* range 1~16 */
  uint32_t line_width;
};

struct rga_req {
  /* (enum) process mode sel */
  uint8_t render_mode;
  /* src image info */
  struct rga_img_info_t src;
  /* dst image info */
  struct rga_img_info_t dst;
  /* patten image info */
  struct rga_img_info_t pat;
  /* rop4 mask addr */
  uint32_t rop_mask_addr;
  /* LUT addr */
  uint32_t LUT_addr;
  /* dst clip window default value is dst_vir */
  struct RECT clip;

  /* value from [0, w-1] / [0, h-1] */
  /* dst angle  default value 0  16.16 scan from table */
  int32_t sina;
  /* dst angle  default value 0  16.16 scan from table */
  int32_t cosa;
  /* alpha rop process flag           */
  uint16_t alpha_rop_flag;

  /* ([0] = 1 alpha_rop_enable)       */
  /* ([1] = 1 rop enable)             */
  /* ([2] = 1 fading_enable)          */
  /* ([3] = 1 PD_enable)              */
  /* ([4] = 1 alpha cal_mode_sel)     */
  /* ([5] = 1 dither_enable)          */
  /* ([6] = 1 gradient fill mode sel) */
  /* ([7] = 1 AA_enable)              */
  /* 0 nearst / 1 bilnear / 2 bicubic */
  uint8_t scale_mode;
  /* color key max */
  uint32_t color_key_max;
  /* color key min */
  uint32_t color_key_min;
  /* foreground color */
  uint32_t fg_color;
  /* background color */
  uint32_t bg_color;
  /* color fill use gradient */
  struct COLOR_FILL gr_color;
  struct line_draw_t line_draw_info;
  struct FADING fading;
  /* porter duff alpha mode sel */
  uint8_t PD_mode;
  /* global alpha value */
  uint8_t alpha_global_value;
  /* rop2/3/4 code  scan from rop code table */
  uint16_t rop_code;
  /* [2] 0 blur 1 sharp / [1:0] filter_type */
  uint8_t bsfilter_flag;
  /* (enum) color palatte  0/1bpp, 1/2bpp 2/4bpp 3/8bpp */
  uint8_t palette_mode;
  /* (enum) BT.601 MPEG / BT.601 JPEG / BT.709  */
  uint8_t yuv2rgb_mode;
  /* 0/big endian 1/little endian */
  uint8_t endian_mode;
  /* (enum) rotate mode  */
  uint8_t rotate_mode;

  /* 0x0,     no rotate  */
  /* 0x1,     rotate     */
  /* 0x2,     x_mirror   */
  /* 0x3,     y_mirror   */
  /* 0 solid color / 1 patten color */
  uint8_t color_fill_mode;
  /* mmu information */
  struct MMU mmu_info;
  /* ([0~1] alpha mode)       */
  uint8_t alpha_rop_mode;

  /* ([2~3] rop   mode)       */
  /* ([4]   zero  mode en)    */
  /* ([5]   dst   alpha mode) */
  uint8_t src_trans_mode;
};

#ifdef __cplusplus
}
#endif

#endif  //__RGA_H__