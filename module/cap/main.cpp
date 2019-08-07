#include <list>

#include "md_camerahal.h"
#include "md_camerabuf.h"
#include "thread_helper.h"
#include "process/md_display_process.h"
#include "process/md_encoder_process.h"
#include "protocol.h"
#include "config.h"

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
    struct rk_cams_dev_info cam_info;
    std::shared_ptr<RKCameraBufferAllocator> ptr_allocator;
    CameraFactory cam_factory;
    std::shared_ptr<RKCameraHal> cam_dev;

    std::shared_ptr<DisplayProcess> display_process;
};

static RKVideo* rkvideo = NULL;
static int init_ok = 0;

RKVideo::RKVideo()
{
    display_state_ = false;

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

void RKVideo::capture(char *file_name)
{
	if (display_state_ == 0)
		return;
	display_process->capture(file_name);
}

int main(int argc, char *argv[])
{
	if (argc < 3)	
		return 0;
	rkvideo = new RKVideo();
	rkvideo->displayLocal();
	int count  = atoi(argv[2]);
	char path[64] = {0};
	for (int i = 0; i < count; ++i) {
		sprintf(path,"%s%s_%d.jpg",FAST_PIC_PATH,argv[1],i);
		printf("path:%s\n", path);
		rkvideo->capture(path);
		usleep(500000);
	}
	delete rkvideo;
	return 0;
}
