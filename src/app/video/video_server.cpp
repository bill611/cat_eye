#include <list>

#include "camerahal.h"
#include "camerabuf.h"
#include "thread_helper.h"
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

	void displayPeer(int w,int h,void* decCallback);
	void displayLocal(void);
	void displayOff(void);
    void faceOnOff(bool type);
	void h264EncOnOff(bool type,int w,int h,EncCallbackFunc encCallback);
	void capture(char *file_name);
	void recordStart(int w,int h,EncCallbackFunc recordCallback);
	void recordSetStopFunc(RecordStopCallbackFunc recordCallback);
	void recordStop(void);

 private:
 	
    int display_state_; // 0关闭 1本地视频 2远程视频
    bool face_state_;
    bool h264enc_state_;
    struct rk_cams_dev_info cam_info;
    std::shared_ptr<RKCameraBufferAllocator> ptr_allocator;
    CameraFactory cam_factory;
    std::shared_ptr<RKCameraHal> cam_dev;

    std::shared_ptr<DisplayProcess> display_process;
    std::shared_ptr<FaceProcess> face_process;
    std::shared_ptr<H264Encoder> encode_process;
};

static RKVideo* rkvideo = NULL;
static int init_ok = 0;

RKVideo::RKVideo()
{
    display_state_ = 0;
	face_state_ = false;
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
	cam_dev->start(5, ptr_allocator);

    display_process = std::make_shared<DisplayProcess>();
	if (display_process.get() == nullptr)
		std::cout << "[rv_video]DisplayProcess make_shared error" << std::endl;

    face_process = std::make_shared<FaceProcess>();
	if (face_process.get() == nullptr)
		std::cout << "[rv_video]FaceProcess make_shared error" << std::endl;

	encode_process = std::make_shared<H264Encoder>();
	if (encode_process.get() == nullptr)
		std::cout << "[rv_video]H264Encoder make_shared error" << std::endl;
	init_ok = 1;
}

RKVideo::~RKVideo()
{
	disconnect(cam_dev->mpath(), display_process);
	disconnect(cam_dev->mpath(), encode_process);
	disconnect(cam_dev->mpath(), face_process);
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
		display_process->showLocalVideo();
		connect(cam_dev->mpath(), display_process, cam_dev->format(), 0, nullptr);
	}
}
void RKVideo::displayPeer(int w,int h,void* decCallback)
{
	if (cam_info.num_camers <= 0)
		return;
	if (display_state_ != 2) {
		display_state_ = 2;
		disconnect(cam_dev->mpath(), display_process);
		display_process->showPeerVideo(w,h,(DecCallbackFunc)decCallback);
	}
} 

void RKVideo::displayOff(void)
{
	if (cam_info.num_camers <= 0)
		return;
	if (display_state_ != 0) {
		display_state_ = 0;
		disconnect(cam_dev->mpath(), display_process);
		display_process->setVideoBlack();
	}
} 
void RKVideo::faceOnOff(bool type)
{
	if (cam_info.num_camers <= 0)
		return;
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
	if (h264enc_state_ == true) {
		encode_process->capture(file_name);
	} else {
		display_process->capture(file_name);
	}
}

void RKVideo::recordStart(int w,int h,EncCallbackFunc recordCallback)
{
    if (display_state_ != 1)
        return ;
	h264EncOnOff(true,w,h,NULL);
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

static void* threadVideoInit(void *arg)
{
	RKVideo* p_video = NULL;
	while (1) {
		p_video = new RKVideo();
		if (init_ok == 0) {
			printf("[%s,%d]\n", __func__,__LINE__);
			delete p_video;
		} else {
			printf("[%s,%d]\n", __func__,__LINE__);
			rkvideo = p_video;
			break;
		}
		sleep(1);
	}
	return NULL;
}

extern "C" 
int rkVideoInit(void)
{
	// createThread(threadVideoInit,NULL);
	rkvideo = new RKVideo();
}

extern "C" 
int rkVideoDisplayLocal(void)
{
	if (rkvideo)
		rkvideo->displayLocal();
}

extern "C" 
int rkVideoDisplayPeer(int w,int h,void * decCallBack)
{
	if (rkvideo)
		rkvideo->displayPeer(w,h,decCallBack);
}
extern "C" 
int rkVideoDisplayOff(void)
{
	if (rkvideo)
		rkvideo->displayOff();
}
extern "C" 
int rkVideoFaceOnOff(int type)
{
	if (rkvideo == NULL)
		return 0;
	if (type)
		rkvideo->faceOnOff(true);
	else
		rkvideo->faceOnOff(false);
}

extern "C" 
int rkH264EncOn(int w,int h,EncCallbackFunc encCallback)
{
	if (rkvideo)
		rkvideo->h264EncOnOff(true,w,h,encCallback);
}

extern "C" 
int rkH264EncOff(void)
{
	if (rkvideo)
		rkvideo->h264EncOnOff(false,0,0,NULL);
}

extern "C" 
int rkVideoCapture(char *file_name)
{
	if (rkvideo)
		rkvideo->capture(file_name);
}
extern "C" 
int rkVideoRecordStart(int w,int h,EncCallbackFunc recordCallback)
{
	if (rkvideo)
		rkvideo->recordStart(w,h,recordCallback);
}
extern "C" 
int rkVideoRecordSetStopFunc(RecordStopCallbackFunc recordCallback)
{
	if (rkvideo)
		rkvideo->recordSetStopFunc(recordCallback);
}
extern "C" 
int rkVideoRecordStop(void)
{
	if (rkvideo)
		rkvideo->recordStop();
}
