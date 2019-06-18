#include <stdio.h>
#include "video.h"
#include "lcd.h"

RKVideo::RKVideo()
{
	printf("[rv_video:%s]\n",__func__);
    memset(&cam_info, 0, sizeof(cam_info));
    CamHwItf::getCameraInfos(&cam_info);
    if (cam_info.num_camers <= 0) {
		printf("[rv_video:%s]fail\n",__func__);
		return ;
	}
	
    ptr_allocator = shared_ptr<RKCameraBufferAllocator>(new RKCameraBufferAllocator());

    display_process = std::make_shared<DisplayProcess>();
	if (display_process.get() == nullptr)
		std::cout << "[rv_video]DisplayProcess make_shared error" << std::endl;
}

RKVideo::~RKVideo()
{
	printf("[rv_video:%s]\n",__func__);

    ptr_allocator.reset();
    display_process.reset();

}

void RKVideo::start(void)
{
	printf("[rv_video:%s]\n",__func__);
	
	for (int i = 0; i < cam_info.num_camers; i++) {
        shared_ptr<CamHwItf> new_dev = cam_factory.GetCamHwItf(&cam_info, i);
		shared_ptr<RKCameraHal> new_cam = ((shared_ptr<RKCameraHal>)
				new RKCameraHal(new_dev, cam_info.cam[i]->index,  cam_info.cam[i]->type));
		if (new_dev.get() != nullptr)
       		camhals.push_back(new_cam);
    }

	std::list<shared_ptr<RKCameraHal>>::iterator it = camhals.begin();
    for (; it != camhals.end(); it++) {
        if ((*it)->type() == RK_CAM_ATTACHED_TO_ISP) {
			printf("[rv_video:%s] ISP_CAMERA\n",__func__);
            (*it)->init(1280, 720, 25);
            (*it)->start(2, ptr_allocator);
            connect((*it)->mpath(), display_process, (*it)->format(), 0, nullptr);

        } else if ((*it)->type() == RK_CAM_ATTACHED_TO_CIF) {
			printf("[rv_video:%s] CIF_CAMERA\n",__func__);
            (*it)->init(1280, 720, 30);
            (*it)->start(2, ptr_allocator);
        }
    }
}

void RKVideo::stop(void)
{
	printf("[rv_video:%s]\n",__func__);
	
	std::list<shared_ptr<RKCameraHal>>::iterator it = camhals.begin();
	for (; it != camhals.end(); it++) {
		if ((*it)->type() == RK_CAM_ATTACHED_TO_ISP) {
        	disconnect((*it)->mpath(), display_process);
		}
		(*it)->stop();
	}
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

int RKVideo::connect(std::shared_ptr<StreamPUBase> pre, 
				std::shared_ptr<StreamPUBase> next,
				frm_info_t &frmFmt, const uint32_t num, 
				std::shared_ptr<RKCameraBufferAllocator> allocator)
{
    if (!pre.get() || !next.get()) {
		printf("[rv_video:%s]StreamPUBase,PU is NULL\n",__func__);
    }

    pre->addBufferNotifier(next.get());
    next->prepare(frmFmt, num, allocator);
    if (!next->start()) {
		printf("[rv_video:%s]StreamPUBase,PU start failed!\n",__func__);
    }

    return 0;
}

void RKVideo::disconnect(std::shared_ptr<CamHwItf::PathBase> mpath, 
					 std::shared_ptr<StreamPUBase> next)
{
    if (!mpath.get() || !next.get())
        return;

	printf("[rv_video:%s]PathBase\n",__func__);
    mpath->removeBufferNotifer(next.get());
    next->stop();
    next->releaseBuffers();
}
void RKVideo::disconnect(std::shared_ptr<StreamPUBase> pre, 
					 std::shared_ptr<StreamPUBase> next)
{
    if (!pre.get() || !next.get())
        return;

	printf("[rv_video:%s]StreamPUBase\n",__func__);
    pre->removeBufferNotifer(next.get());
    next->stop();
    next->releaseBuffers();
}




int video(int argc, char* argv[])
{
	display_init();
	RKVideo* rkvideo = new RKVideo();

	rkvideo->start();

	while(1)
	{
        usleep(200000);
	}

	rkvideo->stop();
	delete rkvideo;
	rkvideo = NULL;

	return 0;
}

