#ifndef _RK_RGA_H_
#define _RK_RGA_H_

enum {
  RGA_FORMAT_RGBA_8888 = 0x0,
  RGA_FORMAT_RGBX_8888 = 0x1,
  RGA_FORMAT_RGB_888 = 0x2,
  RGA_FORMAT_BGRA_8888 = 0x3,
  RGA_FORMAT_RGB_565 = 0x4,
  RGA_FORMAT_RGBA_5551 = 0x5,
  RGA_FORMAT_RGBA_4444 = 0x6,
  RGA_FORMAT_BGR_888 = 0x7,
  RGA_FORMAT_YCBCR_422_SP = 0x8,
  RGA_FORMAT_YCBCR_422_P = 0x9,
  RGA_FORMAT_YCBCR_420_SP = 0xa,
  RGA_FORMAT_YCBCR_420_P = 0xb,
  RGA_FORMAT_YCRCB_422_SP = 0xc,
  RGA_FORMAT_YCRCB_422_P = 0xd,
  RGA_FORMAT_YCRCB_420_SP = 0xe,
  RGA_FORMAT_YCRCB_420_P = 0xf,
  RGA_FORMAT_BPP1 = 0x10,
  RGA_FORMAT_BPP2 = 0x11,
  RGA_FORMAT_BPP4 = 0x12,
  RGA_FORMAT_BPP8 = 0x13,
};

#ifdef __cplusplus
extern "C" {
#endif

int rk_rga_open(void);
void rk_rga_close(int fd);

int rk_rga_ionfdnv12_to_ionfdnv12_rotate_ext(
    int src_fd,
    int src_w,
    int src_h,
    int src_vir_w,
    int src_vir_h,
    int dst_fd,
    int dst_w,
    int dst_h,
    int rotate);

int rk_rga_phyrgb565_to_ionfdrgb565_rotate(int rga_fd,
                           int src_phy,
                           int src_w,
                           int src_h,
                           int dst_fd,
                           int dst_w,
                           int dst_h,
                           int rotate);

int rk_rga_phy_to_ionfd_rotate(int rga_fd,
                           int src_phy,
                           int src_w,
                           int src_h,
                           int dst_fd,
                           int dst_w,
                           int dst_h,
                           int rotate,
                           int src_format,
                           int dst_format);

int rk_rga_ionfdnv12_to_ionfdnv12_rotate(int rga_fd,
                           int src_fd,
                           int src_w,
                           int src_h,
                           int src_vir_w,
                           int src_vir_h,
                           int dst_fd,
                           int dst_w,
                           int dst_h,
                           int rotate);

int rk_rga_ionfdnv12_to_ionfdnv12_scal(int fd,
                     int src_fd,
                     int src_w,
                     int src_h,
                     int dst_fd,
                     int dst_w,
                     int dst_h,
                     int x,
                     int y,
                     int scal_w,
                     int scal_h,
                     int src_vir_w,
                     int src_vir_h);

int rk_rga_ionfdnv12_to_ionfdnv12_scal_ext(int src_fd,
                     int src_w,
                     int src_h,
                     int dst_fd,
                     int dst_w,
                     int dst_h,
                     int x,
                     int y,
                     int scal_w,
                     int scal_h,
                     int src_vir_w,
                     int src_vir_h);

int rk_rga_phyrgb565_to_ionfdrgb565_scal(int rga_fd,
                           int src_phy,
                           int src_w,
                           int src_h,
                           int src_vir_w,
                           int src_vir_h,
                           int dst_fd,
                           int dst_w,
                           int dst_h);

int rk_rga_ionfd_to_ionfd_rotate(int rga_fd,
                           int src_fd,
                           int src_w,
                           int src_h,
                           int src_fmt,
                           int src_vir_w,
                           int src_vir_h,
                           int dst_fd,
                           int dst_w,
                           int dst_h,
                           int dst_fmt,
                           int rotate);

int rk_rga_ionfd_to_ionfd_rotate_ext(
    int src_fd,
    int src_w,
    int src_h,
    int src_fmt,
    int src_vir_w,
    int src_vir_h,
    int dst_fd,
    int dst_w,
    int dst_h,
    int dst_fmt,
    int rotate);

int rk_rga_ionfd_to_ionfd_scal(int fd,
                     int src_fd,
                     int src_w,
                     int src_h,
                     int src_fmt,
                     int dst_fd,
                     int dst_w,
                     int dst_h,
                     int dst_fmt,
                     int x,
                     int y,
                     int scal_w,
                     int scal_h,
                     int src_vir_w,
                     int src_vir_h);

int rk_rga_ionfd_to_ionfd_scal_ext(int src_fd,
                     int src_w,
                     int src_h,
                     int src_fmt,
                     int dst_fd,
                     int dst_w,
                     int dst_h,
                     int dst_fmt,
                     int x,
                     int y,
                     int scal_w,
                     int scal_h,
                     int src_vir_w,
                     int src_vir_h);

int rk_rga_ionfd_to_ionfd_rotate_offset_ext(int rga_fd,
                         int src_fd,
                         int src_w,
                         int src_h,
                         int src_fmt,
                         int src_vir_w,
                         int src_vir_h,
                         int src_act_w,
                         int src_act_h,
                         int src_x_offset,
                         int src_y_offset,
                         int dst_fd,
                         int dst_w,
                         int dst_h,
                         int dst_fmt,
                         int rotate);

int rk_rga_ionphy_to_ionphy_scal(int fd,
                     int src_phy,
                     int src_w,
                     int src_h,
                     int src_fmt,
                     int dst_phy,
                     int dst_w,
                     int dst_h,
                     int dst_fmt,
                     int x,
                     int y,
                     int scal_w,
                     int scal_h,
                     int src_vir_w,
                     int src_vir_h);

int rk_rga_ionfd_to_ionfd_mirror(int rga_fd,
                           int src_fd,
                           int src_w,
                           int src_h,
                           int src_fmt,
                           int src_vir_w,
                           int src_vir_h,
                           int dst_fd,
                           int dst_w,
                           int dst_h,
                           int dst_fmt);

int rk_rga_phy_to_ionfd_offset_rotate(int rga_fd,
                           int src_phy,
                           int src_w,
                           int src_h,
                           int dst_fd,
                           int dst_w,
                           int dst_h,
                           int y_offset,
                           int rotate,
                           int src_format,
                           int dst_format);
#ifdef __cplusplus
}
#endif

#endif
