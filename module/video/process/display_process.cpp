#include "display_process.h"
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
	if (This->capCallbackFunc())
		This->capCallbackFunc()();
	fp = NULL;
}
DisplayProcess::DisplayProcess()
     : StreamPUBase("DisplayProcess", true, true)
{
    rga_fd = rk_rga_open();
    if (rga_fd < 0)
		printf("rk_rga_open failed\n");
	myH264DecInit();
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
	start_dec_ = false;

}
static void* threadH264Dec(void *arg)
{
	DisplayProcess *process = (DisplayProcess *)arg;
	struct win* video_win = rk_fb_getvideowin();
	unsigned char data_in[1024*300];
	unsigned char data_out[1024*600*3/2];
	int out_w = 0,out_h = 0;
    int disp_width = 0, disp_height = 0;
    int out_device = rk_fb_get_out_device(&disp_width, &disp_height);

    while (process->start_dec() == true) {
        int size_in = 0;
        int size_out = 0;
		memset(data_in,0,sizeof(data_in));
		memset(data_out,0,sizeof(data_out));
        if (process->decCallback())
            process->decCallback()(data_in,&size_in);
		unsigned char *nv12_scale_data = NULL;
		if (size_in > 0) {
			if (my_h264dec)
				size_out = my_h264dec->decode(my_h264dec,data_in,size_in,data_out,&out_w,&out_h);
			if (out_w != 0 && out_h != 0 && size_out > 0) {
				NV12Scale(data_out, out_w, out_h, &nv12_scale_data, disp_width, disp_height);
				memcpy(video_win->buffer,nv12_scale_data,disp_width*disp_height*3/2);
				writePicture(process,nv12_scale_data);
				if (rk_fb_video_disp(video_win) < -1){
					printf("rk_fb_video_disp failed\n");
				}
			}
		}
		if (nv12_scale_data)
			free(nv12_scale_data);

		usleep(10000);
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
	setVideoBlack();
	if (my_h264dec)
		my_h264dec->init(my_h264dec,w,h);
	createThread(threadH264Dec,this);
}

void DisplayProcess::capture(CapCallbackFunc capCallbackFunc,char *file_name)
{
	while (fp != NULL) {
		usleep(10000);	
	}
	capCallbackFunc_ = capCallbackFunc;
	if (fp == NULL)
		fp = fopen(file_name,"wb");
}
