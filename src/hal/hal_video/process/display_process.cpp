#include "display_process.h"

DisplayProcess::DisplayProcess()
     : StreamPUBase("DisplayProcess", true, true)
{
    rga_fd = rk_rga_open();
    if (rga_fd < 0)
		printf("rk_rga_open failed\n");
}

DisplayProcess::~DisplayProcess()
{
    rk_rga_close(rga_fd);
}

bool DisplayProcess::processFrame(std::shared_ptr<BufferBase> inBuf,
                                        std::shared_ptr<BufferBase> outBuf)
{
    int src_w = inBuf->getWidth();
    int src_h = inBuf->getHeight();
    int src_fd = (int)(inBuf->getFd());

    int vir_w = src_w;
    int vir_h = src_h;
    int src_fmt = RGA_FORMAT_YCBCR_420_SP;

    int disp_width = 0, disp_height = 0;
    struct win* video_win = rk_fb_getvideowin();

    int out_device = rk_fb_get_out_device(&disp_width, &disp_height);

    int dst_fd = video_win->video_ion.fd;
    int dst_fmt = RGA_FORMAT_YCBCR_420_SP;

    int dst_w = disp_width;
    int dst_h = disp_height;
    //int rotate_angle = (out_device == OUT_DEVICE_HDMI ? 0 : VIDEO_DISPLAY_ROTATE_ANGLE);
	int rotate_angle = (out_device == OUT_DEVICE_HDMI ? 0 : 0);

    int ret = rk_rga_ionfd_to_ionfd_rotate(rga_fd,
                                           src_fd, src_w, src_h, src_fmt, vir_w, vir_h,
                                           dst_fd, dst_w, dst_h, dst_fmt,
                                           rotate_angle);
    if (ret) {
        printf("rk_rga_ionfd_to_ionfd_rotate failed\n");
        return false;
    }

#ifdef TEST_WRITE_SP_TO_FILE
      //write to file
      char fname[50] = {0};
      static int frames = 0;
      snprintf(fname, 30, "/tmp/yuv_%dx%d.yuv", src_w, src_h);
      frames++;
      if ((frames > 25) && (frames < 31)) {
        FILE* yuv_file =  fopen(fname, "a+");
        if (yuv_file) {
          fwrite(inBuf->getVirtAddr(), inBuf->getDataSize(), 1, yuv_file);
          printf("write 0x%x bytes to file!", inBuf->getDataSize());
          fclose(yuv_file);
        } else
          printf("open file %s error", fname);
      }
#endif

    if (rk_fb_video_disp(video_win) < -1){
		printf("rk_fb_video_disp failed\n");
	}

    return true;
}

