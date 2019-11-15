#include "display_process.h"
#include "thread_helper.h"
#include "h264_enc_dec/mpi_dec_api.h"
#include "jpeg_enc_dec.h"
#include "libyuv.h"

#define CAMMER_TICK_NUM 5
static FILE *fp = NULL;
static int cammer_tick = CAMMER_TICK_NUM;  // 摄像头正常连接计数，当计数为0，判断摄像头接线异常
static bool cammer_check_thread = false;
int NV12Scale(unsigned char *psrc_buf, int psrc_w, int psrc_h, unsigned char **pdst_buf, int pdst_w, int pdst_h);
static void writePicture(unsigned char *data,int w,int h)
{
	if (fp == NULL)
		return;
	unsigned char *jpeg_buf = NULL;
	int size = 0;
	yuv420spToJpeg(data,w,h,&jpeg_buf,&size);
	if (jpeg_buf) {
		fwrite(jpeg_buf,1,size,fp);
		fflush(fp);
		fclose(fp);
		free(jpeg_buf);
	}
	fp = NULL;
}
static void* threadCheckCammer(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	cammer_check_thread = true;
	DisplayProcess *process = (DisplayProcess *)arg;
	while (1) {
		// printf("dec:%d,tick:%d\n",process->start_dec(),cammer_tick );
		if (process->start_dec() == false && cammer_tick > 0) {
			if (--cammer_tick == 0) {
				process->setCammerState(0);
				break;
			}
		}
		sleep(1);
	}
	cammer_check_thread = false;
	return NULL;
}

DisplayProcess::DisplayProcess()
     : StreamPUBase("DisplayProcess", true, true)
{
    rga_fd = rk_rga_open();
    if (rga_fd < 0)
		printf("rk_rga_open failed\n");

	cammer_state_ = 1;
	cammer_tick = CAMMER_TICK_NUM;
	myH264DecInit();
	if (cammer_check_thread == false)
		createThread(threadCheckCammer,this);
}

DisplayProcess::~DisplayProcess()
{
    rk_rga_close(rga_fd);
	cammer_state_ = 0;
}

bool DisplayProcess::processFrame(std::shared_ptr<BufferBase> inBuf,
                                        std::shared_ptr<BufferBase> outBuf)
{
	cammer_tick = CAMMER_TICK_NUM;
    int src_w = inBuf->getWidth();
    int src_h = inBuf->getHeight();
    int src_fd = (int)(inBuf->getFd());

    int vir_w = src_w;
    int vir_h = src_h;
    int src_fmt = RGA_FORMAT_YCBCR_420_SP;

    int screen_width = 0, screen_height = 0;
    struct win* video_win = rk_fb_getvideowin();

    int out_device = rk_fb_get_out_device(&screen_width, &screen_height);

    int dst_fd = video_win->video_ion.fd;
    int dst_fmt = RGA_FORMAT_YCBCR_420_SP;

    int dst_w = screen_width;
    int dst_h = screen_height;
	int rotate_angle = (out_device == OUT_DEVICE_HDMI ? 0 : 0);

    int ret = rk_rga_ionfd_to_ionfd_rotate(rga_fd,
                                           src_fd, src_w, src_h, src_fmt, vir_w, vir_h,
                                           dst_fd, dst_w, dst_h, dst_fmt,
                                           rotate_angle);
    if (ret) {
        printf("rk_rga_ionfd_to_ionfd_rotate failed\n");
        return false;
    }
	writePicture((unsigned char *)inBuf->getVirtAddr(),1280,720);

    if (rk_fb_video_disp(video_win) < -1){
		printf("rk_fb_video_disp failed\n");
	}

    return true;
}

void DisplayProcess::setVideoBlack()
{
    int screen_width = 0, screen_height = 0;
    struct win* video_win = rk_fb_getvideowin();

    int out_device = rk_fb_get_out_device(&screen_width, &screen_height);

	int screen_size = screen_width*screen_height;
	memset(video_win->buffer,0,screen_size);
	memset(video_win->buffer + screen_size,0x80,screen_size/2);

    if (rk_fb_video_disp(video_win) < -1){
		printf("rk_fb_video_disp failed\n");
	}
	cammer_tick = 0;
}
void DisplayProcess::showLocalVideo(void)
{
	start_dec_ = false;
}

static void* threadH264Dec(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	DisplayProcess *process = (DisplayProcess *)arg;
	struct win* video_win = rk_fb_getvideowin();
	unsigned char data_in[1024*200];
	unsigned char data_out[1280*720*3/2];
	int out_w = 0,out_h = 0;
    int screen_width = 0, screen_height = 0;
    int out_device = rk_fb_get_out_device(&screen_width, &screen_height);

#ifdef USE_UDPTALK
	if (my_h264dec)
		my_h264dec->init(my_h264dec,process->getWidth(),process->getHeight());
#endif
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

				int disp_width = out_w * screen_height / out_h;
				if (disp_width < 600)
					disp_width = 600;
				else if (disp_width > 1024)
					disp_width = 1024;
				NV12Scale(data_out, out_w, out_h, &nv12_scale_data, disp_width, screen_height);
				int y_size = disp_width*screen_height;
				int i;
				// Y
				// 居中显示，图像起点
				int disp_start_x = (screen_width - disp_width) / 2;
				int data_index = 0;
				for (i = 0; i < screen_height; ++i) {
					memset(video_win->buffer + i*screen_width,0,disp_start_x);
					memcpy(video_win->buffer + i*screen_width + disp_start_x,&nv12_scale_data[disp_width * data_index++],disp_width);
					memset(video_win->buffer + i*screen_width + disp_start_x + disp_width,0,disp_start_x);
				}
				// uv
				int k ;
				for (k = 0; k < screen_height / 2; ++k,++i) {
					memset(video_win->buffer + i*screen_width,0x80,disp_start_x);
					memcpy(video_win->buffer + i*screen_width + disp_start_x,&nv12_scale_data[disp_width * data_index++],disp_width);
					memset(video_win->buffer + i*screen_width + disp_start_x + disp_width,0x80,disp_start_x);
				}

				writePicture(nv12_scale_data,disp_width,screen_height);
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
	createThread(threadH264Dec,this);
}

void DisplayProcess::capture(char *file_name)
{
	while (fp != NULL) {
		usleep(10000);
	}
	if (fp == NULL)
		fp = fopen(file_name,"wb");
}
