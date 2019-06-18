#include "camerahal.h"

RKCameraHal::RKCameraHal(std::shared_ptr<CamHwItf> dev, 
							   int index, int type)
	:camdev(dev)
	,index_(index)
	,type_(type)
{
	printf("[RKCameraHal:%s]\n",__func__);

	if (camdev->initHw(index) == false)
		printf("[RKCameraHal:%s] camdev initHw error\n",__func__);
    mpath_ = camdev->getPath(CamHwItf::MP);
}

RKCameraHal::~RKCameraHal(void) 
{
	printf("[RKCameraHal:%s] \n",__func__);
}

void RKCameraHal::init(const uint32_t width, 
						 const uint32_t height, const int fps)
{
	printf("[RKCameraHal:%s] \n",__func__);
	format_.frmSize.width = width;
    format_.frmSize.height = height;
    format_.frmFmt = HAL_FRMAE_FMT_NV12;
    format_.colorSpace = HAL_COLORSPACE_JPEG;
    format_.fps = fps;

    HAL_FPS_INFO_t fps_info;
    fps_info.numerator = 1;
    fps_info.denominator = fps;

    if (!camdev->setFps(fps_info))
        printf("[RKCameraHal:%s]dev set fps is %.2f\n", __func__,
                1.0 * fps_info.denominator / fps_info.numerator);
}

void RKCameraHal::start(const uint32_t num, 
                           std::shared_ptr<RKCameraBufferAllocator> ptr_allocator)
{
	printf("[RKCameraHal:%s] \n",__func__);

	if (mpath()->prepare(format_, num, *ptr_allocator, false, 0) == false) {
		printf("[RKCameraHal:%s] mpath prepare failed \n",__func__);
        return;
    }

    if (!mpath()->start()) {
		printf("[RKCameraHal:%s] mpath start failed \n",__func__);
        return;
    }
}

void RKCameraHal::stop(void)
{
	printf("[RKCameraHal:%s] \n",__func__);

    if (mpath().get()) {
        mpath()->stop();
        mpath()->releaseBuffers();
    }
}
