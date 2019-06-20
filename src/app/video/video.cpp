#include "video.h"

static RKVideo* rkvideo = NULL;
static int video_state = 0;
RKVideo::RKVideo()
{
	printf("[rv_video:%s]\n",__func__);
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
	cam_dev->start(2, ptr_allocator);

    display_process = std::make_shared<DisplayProcess>();
	if (display_process.get() == nullptr)
		std::cout << "[rv_video]DisplayProcess make_shared error" << std::endl;
    cammer_process = std::make_shared<CammerProcess>();
	if (cammer_process.get() == nullptr)
		std::cout << "[rv_video]CammerProcess make_shared error" << std::endl;
    encode_process = std::make_shared<H264Encoder>(1280,720);
    if (encode_process.get() == nullptr)
        std::cout << "[rv_video]CammerProcess make_shared error" << std::endl;

	connect(cam_dev->mpath(), cammer_process, cam_dev->format(), 0, nullptr);
}

RKVideo::~RKVideo()
{
	printf("[rv_video:%s]\n",__func__);
	cam_dev->stop();
    ptr_allocator.reset();
    display_process.reset();
	encode_process.reset();

}

void RKVideo::start(void)
{
	connect(cam_dev->mpath(), display_process, cam_dev->format(), 0, nullptr);
	connect(cam_dev->mpath(), encode_process, cam_dev->format(), 0, nullptr);
}

void RKVideo::stop(void)
{
	disconnect(cam_dev->mpath(), display_process);
	disconnect(cam_dev->mpath(), encode_process);
	display_process->setVideoBlack();
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

void RKVideo::startRecord(void)
{
	encode_process->StartYuv();
}
extern "C" 
int rkVideoInit(void)
{
	rkvideo = new RKVideo();
	return 0;
}

extern "C" 
int rkVideoStart(void)
{
	if (video_state == 1)
		return 0;
	video_state = 1;
	rkvideo->start();
	return 0;
}

extern "C" 
int rkVideoStop(void)
{
	if (video_state == 0)
		return 0;
	video_state = 0;
	rkvideo->stop();
}


extern "C" 
int rkVideoStopCapture(void)
{
}

extern "C" 
int rkVideoStartRecord(void)
{
	rkvideo->startRecord();
}

extern "C" 
int rkVideoStopRecord(void)
{
}
