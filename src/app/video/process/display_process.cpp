#include "display_process.h"
#include "thread_helper.h"
#include "h264_enc_dec/mpi_dec_api.h"

//#define TEST_WRITE_SP_TO_FILE

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

    if (rk_fb_video_disp(video_win) < -1){
		printf("rk_fb_video_disp failed\n");
	}

    return true;
}

void DisplayProcess::setVideoBlack()
{
    int disp_width = 0, disp_height = 0;
    struct win* video_win = rk_fb_getvideowin();

    int out_device = rk_fb_get_out_device(&disp_width, &disp_height);

	int screen_size = disp_width*disp_height;
	memset(video_win->buffer,0,screen_size);
	memset(video_win->buffer + screen_size,0x80,screen_size/2);

    if (rk_fb_video_disp(video_win) < -1){
		printf("rk_fb_video_disp failed\n");
	}
}
void DisplayProcess::showLocalVideo(void)
{

}
static void* threadH264Dec(void *arg)
{
	DisplayProcess *process = (DisplayProcess *)arg;
    while (process->start_dec() == true) {
        unsigned char *data_in = NULL;
        unsigned char *data_out = NULL;
        int size = 0;
        if (process->decCallback())
            process->decCallback()((void **)&data_in,&size);

        if (my_h264dec)
            my_h264dec->decode(my_h264dec,data_in,&data_out);

        struct win* video_win = rk_fb_getvideowin();
        if (size)
            memcpy(video_win->video_ion.buffer,data_out,size);

        if (rk_fb_video_disp(video_win) < -1){
            printf("rk_fb_video_disp failed\n");
        }
    }
	if (my_h264dec)
		my_h264dec->unInit(my_h264dec);
	printf("%s(),%d\n", __func__,__LINE__);
	return NULL;
}
void DisplayProcess::showPeerVideo(int w,int h,DecCallbackFunc decCallback)
{
	width_ = w;
	height_ = h;
	decCallback_ = decCallback;
	start_dec_ = true;
	last_frame_ = false;
	createThread(threadH264Dec,this);
}
