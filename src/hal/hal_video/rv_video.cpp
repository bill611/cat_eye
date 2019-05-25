#include "video.h"
#include "lcd.h"
#include "thread_helper.h"

RKVideo::RKVideo()
{
    printf("vicent --------------- %s\n", __FUNCTION__);
    memset(&cam_info, 0, sizeof(cam_info));
    CamHwItf::getCameraInfos(&cam_info);
    if (cam_info.num_camers <= 0) {
		printf("%s: cameras no exist\n", __FUNCTION__);
		exit(-1);
	}
	
    ptr_allocator = shared_ptr<RKCameraBufferAllocator>(new RKCameraBufferAllocator());

	printf("vicent ------------------ %d\n", cam_info.num_camers);

    display_process = std::make_shared<DisplayProcess>();
	if (display_process.get() != nullptr)
		printf("DisplayProcess make_shared error\n");
}

RKVideo::~RKVideo()
{
    printf("vicent --------------- %s\n", __FUNCTION__);

    ptr_allocator.reset();
    display_process.reset();

}

void RKVideo::start(void)
{
    printf("vicent --------------- %s\n", __FUNCTION__);
	
	for (int i = 0; i < cam_info.num_camers; i++) {
        shared_ptr<CamHwItf> new_dev = cam_factory.GetCamHwItf(&cam_info, i);
    	printf("vicent ---------------new_dev  %ld\n", new_dev);
		shared_ptr<RKCameraHal> new_cam = ((shared_ptr<RKCameraHal>)
				new RKCameraHal(new_dev, cam_info.cam[i]->index,  cam_info.cam[i]->type));
		if (new_dev.get() != nullptr)
       		camhals.push_back(new_cam);
    }

	std::list<shared_ptr<RKCameraHal>>::iterator it = camhals.begin();
    for (; it != camhals.end(); it++) {
        if ((*it)->type() == RK_CAM_ATTACHED_TO_ISP) {
			printf("vicent ------------------ ISP_CAMERA\n");
            (*it)->init(1280, 720, 25);
            (*it)->start(2, ptr_allocator);
            connect((*it)->mpath(), display_process, (*it)->format(), 0, nullptr);

        } else if ((*it)->type() == RK_CAM_ATTACHED_TO_CIF) {
			printf("vicent ------------------ CIF_CAMERA\n");
            (*it)->init(1280, 720, 30);
            (*it)->start(2, ptr_allocator);
        }
    }
}

void RKVideo::stop(void)
{
    printf("vicent --------------- %s\n", __FUNCTION__);
	
	std::list<shared_ptr<RKCameraHal>>::iterator it = camhals.begin();
	for (; it != camhals.end(); it++) {
		if ((*it)->type() == RK_CAM_ATTACHED_TO_ISP) {
        	disconnect((*it)->mpath(), display_process);
		}
		(*it)->stop();
		(*it)->stop();
	}
}

int RKVideo::connect(std::shared_ptr<CamHwItf::PathBase> mpath, 
				std::shared_ptr<StreamPUBase> next,
				frm_info_t& frmFmt, const uint32_t num, 
				std::shared_ptr<RKCameraBufferAllocator> allocator)
{			
	if (!mpath.get() || !next.get()) {
		printf("%s: PU is NULL\n", __func__);
	}

	mpath->addBufferNotifier(next.get());
	next->prepare(frmFmt, num, allocator);
	if (!next->start()) {
		printf("PU start failed!\n");
	}

	return 0;
}

int RKVideo::connect(std::shared_ptr<StreamPUBase> pre, 
				std::shared_ptr<StreamPUBase> next,
				frm_info_t &frmFmt, const uint32_t num, 
				std::shared_ptr<RKCameraBufferAllocator> allocator)
{
    if (!pre.get() || !next.get()) {
        printf("%s: PU is NULL\n", __func__);
    }

    pre->addBufferNotifier(next.get());
    next->prepare(frmFmt, num, allocator);
    if (!next->start()) {
        printf("PU start failed!\n");
    }

    return 0;
}

void RKVideo::disconnect(std::shared_ptr<CamHwItf::PathBase> mpath, 
					 std::shared_ptr<StreamPUBase> next)
{
    if (!mpath.get() || !next.get())
        return;

    mpath->removeBufferNotifer(next.get());
    next->stop();
    next->releaseBuffers();
}
void RKVideo::disconnect(std::shared_ptr<StreamPUBase> pre, 
					 std::shared_ptr<StreamPUBase> next)
{
    if (!pre.get() || !next.get())
        return;

    pre->removeBufferNotifer(next.get());
    next->stop();
    next->releaseBuffers();
}




static RKVideo* rkvideo = new RKVideo();

static void* videoStartThead(void *)
{
	// display_init();
	if (rkvideo == NULL)
		rkvideo = new RKVideo();

	rkvideo->start();
	return NULL;
}

extern "C" 
int video_init(void)
{
	createThread(videoStartThead,NULL);
	// display_init();
	// if (rkvideo == NULL)
		// rkvideo = new RKVideo();

	// rkvideo->start();

	// while(1)
	// {
        // usleep(200000);
	// }

	// rkvideo->stop();
	// delete rkvideo;
	// rkvideo = NULL;

	return 0;
}

extern "C" 
int video_uninit(void)
{
	rkvideo->stop();
}
