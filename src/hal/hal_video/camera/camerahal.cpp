#include "camerahal.h"

RKCameraHal::RKCameraHal(std::shared_ptr<CamHwItf> dev, 
							   int index, int type)
	:camdev(dev)
	,index_(index)
	,type_(type)
{
    printf("vicent --------------- %s\n", __FUNCTION__);

	if (camdev->initHw(index) == false)
	    printf("vicent ---------------camdev initHw error\n");
    mpath_ = camdev->getPath(CamHwItf::MP);
}

RKCameraHal::~RKCameraHal(void) 
{
    printf("vicent --------------- %s\n", __FUNCTION__);
}

void RKCameraHal::init(const uint32_t width, 
						 const uint32_t height, const int fps)
{
    printf("vicent --------------- %s 1\n", __FUNCTION__);

	format_.frmSize.width = width;
    format_.frmSize.height = height;
    format_.frmFmt = HAL_FRMAE_FMT_NV12;
    format_.colorSpace = HAL_COLORSPACE_JPEG;
    format_.fps = fps;

    HAL_FPS_INFO_t fps_info;
    fps_info.numerator = 1;
    fps_info.denominator = fps;

    if (!camdev->setFps(fps_info))
        printf("%s: dev set fps is %.2f\n", __func__,
                1.0 * fps_info.denominator / fps_info.numerator);
}

void RKCameraHal::start(const uint32_t num, 
                           std::shared_ptr<RKCameraBufferAllocator> ptr_allocator)
{
    printf("vicent --------------- %s\n", __FUNCTION__);

	if (mpath()->prepare(format_, num, *ptr_allocator, false, 0) == false) {
        printf("mpath prepare failed.\n");
        return;
    }

    if (!mpath()->start()) {
        printf("mpath start failed.\n");
        return;
    }
}

void RKCameraHal::stop(void)
{
    printf("vicent --------------- %s\n", __FUNCTION__);

    if (mpath().get()) {
        mpath()->stop();
        mpath()->releaseBuffers();
    }
}
