#include <list>

#include "camerahal.h"
#include "camerabuf.h"
#include "process/display_process.h"
#include "process/face_process.h"
#include "process/encoder_process.h"


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

    void displayOnOff(bool type);
    void faceOnOff(bool type);
	void h264EncOnOff(bool type,EncCallbackFunc callback);

    void startRecord(void);
 private:
 	
    bool display_state_;
    bool face_state_;
    struct rk_cams_dev_info cam_info;
    std::shared_ptr<RKCameraBufferAllocator> ptr_allocator;
    CameraFactory cam_factory;
    std::shared_ptr<RKCameraHal> cam_dev;

    std::shared_ptr<DisplayProcess> display_process;
    std::shared_ptr<FaceProcess> face_process;
    std::shared_ptr<H264Encoder> encode_process;
};

static RKVideo* rkvideo = NULL;

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
	cam_dev->start(2, ptr_allocator);

    display_process = std::make_shared<DisplayProcess>();
	if (display_process.get() == nullptr)
		std::cout << "[rv_video]DisplayProcess make_shared error" << std::endl;

    face_process = std::make_shared<FaceProcess>();
	if (face_process.get() == nullptr)
		std::cout << "[rv_video]FaceProcess make_shared error" << std::endl;

	encode_process = std::make_shared<H264Encoder>();
	if (encode_process.get() == nullptr)
		std::cout << "[rv_video]H264Encoder make_shared error" << std::endl;

	// connect(cam_dev->mpath(), face_process, cam_dev->format(), 0, nullptr);
}

RKVideo::~RKVideo()
{
	printf("[rv_video:%s]\n",__func__);
	cam_dev->stop();

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

void RKVideo::displayOnOff(bool type)
{
    if (type == true) {
        if (display_state_ == false) {
            display_state_ = true;
            connect(cam_dev->mpath(), display_process, cam_dev->format(), 0, nullptr);
        }
    } else {
        if (display_state_ == true) {
            display_state_ = false;
            disconnect(cam_dev->mpath(), display_process);
            display_process->setVideoBlack();
        }
	}
}
void RKVideo::faceOnOff(bool type)
{
    if (type == true) {
        if (face_state_ == false) {
            face_state_ = true;
            face_process->faceInit();
            connect(cam_dev->mpath(), face_process, cam_dev->format(), 0, nullptr);
        }
    } else {
        if (face_state_ == true) {
            face_state_ = false;
            disconnect(cam_dev->mpath(), face_process);
            face_process->faceUnInit();
        }
	}
}
void RKVideo::h264EncOnOff(bool type,EncCallbackFunc callback)
{
	if (type == true) {
		connect(cam_dev->mpath(), encode_process, cam_dev->format(), 0, nullptr);
		encode_process->startEnc(1280,720,callback);
	} else {
		encode_process->stopEnc();
		disconnect(cam_dev->mpath(), encode_process);
	}
}

void RKVideo::startRecord(void)
{
	// encode_process->Start(1280,720);
}

extern "C" 
int rkVideoInit(void)
{
	rkvideo = new RKVideo();
	return 0;
}

extern "C" 
int rkVideoDisplayOnOff(int type)
{
	if (type)
		rkvideo->displayOnOff(true);
	else
		rkvideo->displayOnOff(false);
	return 0;
}

extern "C" 
int rkVideoFaceOnOff(int type)
{
	if (type)
		rkvideo->faceOnOff(true);
	else
		rkvideo->faceOnOff(false);
}

extern "C" 
int rkVideoStop(void)
{
	// rkvideo->stop();
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
