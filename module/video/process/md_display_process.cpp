#include "md_display_process.h"
#include "thread_helper.h"
#include "h264_enc_dec/mpi_dec_api.h"
#include "jpeg_enc_dec.h"
#include "libyuv.h"

static FILE *fp = NULL;
int NV12Scale(unsigned char *psrc_buf, int psrc_w, int psrc_h, unsigned char **pdst_buf, int pdst_w, int pdst_h);
static void writePicture(DisplayProcess *This,unsigned char *data)
{
	if (fp == NULL)
		return;
	unsigned char *jpeg_buf = NULL;
	int size = 0;
	yuv420spToJpeg(data,1280,720,&jpeg_buf,&size);
	if (jpeg_buf) {
		fwrite(jpeg_buf,1,size,fp);
		fflush(fp);
		fclose(fp);
		free(jpeg_buf);
	}
	fp = NULL;
}
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
	int rotate_angle = (out_device == OUT_DEVICE_HDMI ? 0 : 0);

    int ret = rk_rga_ionfd_to_ionfd_rotate(rga_fd,
                                           src_fd, src_w, src_h, src_fmt, vir_w, vir_h,
                                           dst_fd, dst_w, dst_h, dst_fmt,
                                           rotate_angle);
    if (ret) {
        printf("rk_rga_ionfd_to_ionfd_rotate failed\n");
        return false;
    }
	writePicture(this,(unsigned char *)inBuf->getVirtAddr());

    if (rk_fb_video_disp(video_win) < -1){
		printf("rk_fb_video_disp failed\n");
	}
    return true;
}


void DisplayProcess::showLocalVideo(void)
{

}
void DisplayProcess::capture(char *file_name)
{
	fp = fopen(file_name,"wb");
}
