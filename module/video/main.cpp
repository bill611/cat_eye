#include <list>

#include "md_camerahal.h"
#include "md_camerabuf.h"
#include "thread_helper.h"
#include "process/md_display_process.h"
#include "process/md_encoder_process.h"
#include "protocol.h"
#include "config.h"
#include "ipc_server.h"
#include "queue.h"

#define COLOR_KEY_R 0x0
#define COLOR_KEY_G 0x0
#define COLOR_KEY_B 0x1

static IpcServer* ipc_video = NULL;
static Queue *main_queue = NULL;

class RKVideo {
 public:
    RKVideo();
    ~RKVideo();

	int connect(std::shared_ptr<CamHwItf::PathBase> mpath,
		            std::shared_ptr<StreamPUBase> next,
                    frm_info_t& frmFmt, const uint32_t num,
                    std::shared_ptr<RKCameraBufferAllocator> allocator);
    void disconnect(std::shared_ptr<CamHwItf::PathBase> mpath,
		                 std::shared_ptr<StreamPUBase> next);

	void displayLocal(void);
	void h264EncOnOff(bool type,int w,int h,EncCallbackFunc encCallback);
	void capture(char *file_name);
	void recordStart(EncCallbackFunc recordCallback);
	void recordSetStopFunc(RecordStopCallbackFunc recordCallback);
	void recordStop(void);

 private:

    int display_state_; // 0关闭 1本地视频 2远程视频
    bool h264enc_state_;
    struct rk_cams_dev_info cam_info;
    std::shared_ptr<RKCameraBufferAllocator> ptr_allocator;
    CameraFactory cam_factory;
    std::shared_ptr<RKCameraHal> cam_dev;

    std::shared_ptr<DisplayProcess> display_process;
    std::shared_ptr<H264Encoder> encode_process;
};

static RKVideo* rkvideo = NULL;
static int init_ok = 0;

RKVideo::RKVideo()
{
    display_state_ = false;
	h264enc_state_ = false;

    memset(&cam_info, 0, sizeof(cam_info));
    CamHwItf::getCameraInfos(&cam_info);
    if (cam_info.num_camers <= 0) {
		printf("[rv_video:%s]fail\n",__func__);
		return ;
	}
	shared_ptr<CamHwItf> new_dev = cam_factory.GetCamHwItf(&cam_info, 0);
	cam_dev = ((shared_ptr<RKCameraHal>)
			new RKCameraHal(new_dev, cam_info.cam[0]->index,  cam_info.cam[0]->type));
	cam_dev->init(1280, 720, 25);

    ptr_allocator = shared_ptr<RKCameraBufferAllocator>(new RKCameraBufferAllocator());
	cam_dev->start(4, ptr_allocator);

    display_process = std::make_shared<DisplayProcess>();
	if (display_process.get() == nullptr)
		std::cout << "[rv_video]DisplayProcess make_shared error" << std::endl;

	encode_process = std::make_shared<H264Encoder>();
	if (encode_process.get() == nullptr)
		std::cout << "[rv_video]H264Encoder make_shared error" << std::endl;
	init_ok = 1;
}

RKVideo::~RKVideo()
{
	disconnect(cam_dev->mpath(), display_process);
	if (cam_dev)
		cam_dev->stop();
	init_ok = 0;
}

int RKVideo::connect(std::shared_ptr<CamHwItf::PathBase> mpath,
				std::shared_ptr<StreamPUBase> next,
				frm_info_t& frmFmt, const uint32_t num,
				std::shared_ptr<RKCameraBufferAllocator> allocator)
{
	if (!mpath.get() || !next.get()) {
		printf("[rv_video:%s]PathBase,PU is NULL\n",__func__);
	}

	mpath->addBufferNotifier(next.get());
	next->prepare(frmFmt, num, allocator);
	if (!next->start()) {
		printf("[rv_video:%s]PathBase,PU start failed!\n",__func__);
	}

	return 0;
}


void RKVideo::disconnect(std::shared_ptr<CamHwItf::PathBase> mpath,
					 std::shared_ptr<StreamPUBase> next)
{
	if (!mpath.get() || !next.get()) {
		printf("[rv_video:%s]PathBase,PU is NULL\n",__func__);
        return;
	}

    mpath->removeBufferNotifer(next.get());
    next->stop();
    next->releaseBuffers();
}

void RKVideo::displayLocal(void)
{
	if (cam_info.num_camers <= 0)
		return;
	if (display_state_ != 1) {
		display_state_ = 1;
		connect(cam_dev->mpath(), display_process, cam_dev->format(), 0, nullptr);
	}
}
void RKVideo::h264EncOnOff(bool type,int w,int h,EncCallbackFunc encCallback)
{
	if (cam_info.num_camers <= 0)
		return;
	if (type == true) {
		if (h264enc_state_ == false) {
			h264enc_state_ = true;
			connect(cam_dev->mpath(), encode_process, cam_dev->format(), 1, ptr_allocator);
			encode_process->startEnc(w,h,encCallback);
		}
	} else {
		if (h264enc_state_ == true) {
			h264enc_state_ = false;
			encode_process->stopEnc();
			disconnect(cam_dev->mpath(), encode_process);
		}
	}
}
void RKVideo::capture(char *file_name)
{
	if (display_state_ == 0)
		return;
	display_process->capture(file_name);
}
void RKVideo::recordStart(EncCallbackFunc recordCallback)
{
    if (display_state_ != 1)
        return ;
	h264EncOnOff(true,320,240,NULL);
    encode_process->recordStart(recordCallback);
}

void RKVideo::recordSetStopFunc(RecordStopCallbackFunc recordStopCallback)
{
    encode_process->recordSetStopFunc(recordStopCallback);
}

void RKVideo::recordStop(void)
{
    encode_process->recordStop();
	h264EncOnOff(false,0,0,NULL);
}



static int rkH264EncOn(int w,int h,EncCallbackFunc encCallback)
{
	if (rkvideo)
		rkvideo->h264EncOnOff(true,w,h,encCallback);
}

static int rkH264EncOff(void)
{
	if (rkvideo)
		rkvideo->h264EncOnOff(false,0,0,NULL);
}

static int rkVideoCapture(char *file_name)
{
	if (rkvideo)
		rkvideo->capture(file_name);
}
static int rkVideoRecordStart(EncCallbackFunc recordCallback)
{
	if (rkvideo)
		rkvideo->recordStart(recordCallback);
}
static int rkVideoRecordSetStopFunc(RecordStopCallbackFunc recordCallback)
{
	if (rkvideo)
		rkvideo->recordSetStopFunc(recordCallback);
}
static int rkVideoRecordStop(void)
{
	if (rkvideo)
		rkvideo->recordStop();
}
static void display_clean_uiwin(void)
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

static void callbackEncode(void *data,int size,int fram_type)
{
}


static void callbackCapture(void)
{
	IpcData ipc_cap;
	ipc_cap.cmd = IPC_VIDEO_CAPTURE_END;
	main_queue->post(main_queue,&ipc_cap);
}

static void callbackIpc(char *data,int size )
{
	IpcData ipc_data;
	memcpy(&ipc_data,data,sizeof(IpcData));
	printf("[%s]%d\n",__func__,ipc_data.cmd);
	switch(ipc_data.cmd)
	{
		case IPC_VIDEO_ENCODE_ON:
			rkH264EncOn(320,240,callbackEncode);
			break;
		case IPC_VIDEO_ENCODE_OFF:
			rkH264EncOff();
			break;
		case IPC_UART_CAPTURE:
			for (int i = 0; i < ipc_data.count; ++i) {
				char path[128];
				sprintf(path,"%s%s_%d.jpg",FAST_PIC_PATH,ipc_data.data.file.path,i);
				rkVideoCapture(path);
				usleep(500000);
			}
			break;
		case IPC_VIDEO_RECORD_START:
			break;
		case IPC_VIDEO_RECORD_STOP:
			break;
		default:
			break;
	}
}
static void* threadIpcSendMain(void *arg)
{
	Queue * queue = (Queue *)arg;
	waitIpcOpen(IPC_MAIN);
	IpcData ipc_data;
	while (1) {
		queue->get(queue,&ipc_data);	
		ipc_data.dev_type = IPC_DEV_TYPE_VIDEO;
		if (ipc_video)
			ipc_video->sendData(ipc_video,IPC_MAIN,&ipc_data,sizeof(IpcData));
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	rk_fb_init(FB_FORMAT_BGRA_8888);
	rk_fb_set_yuv_range(CSC_BT601F);
	display_clean_uiwin();
	rkvideo = new RKVideo();
	if (rkvideo)
		rkvideo->displayLocal();
	main_queue = queueCreate("main_queue",QUEUE_BLOCK,sizeof(IpcData));
	createThread(threadIpcSendMain,main_queue);
	ipc_video = ipcCreate(IPC_CAMMER,callbackIpc);
	while (access(IPC_MAIN,0) != 0) {
		usleep(10000);	
	}
	delete rkvideo;
	remove(IPC_CAMMER);
	return 0;
}
