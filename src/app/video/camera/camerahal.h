#ifndef __CAMERA_HAL_H__
#define __CAMERA_HAL_H__

#include "camera_factory.h"
#include "camerabuf.h"
#include "display_process.h"

class RKCameraHal  {
 public:

	int type(void)   {
        return type_;
    }
	int index(void)   {
        return index_;
    }
	virtual frm_info_t& format(void) {
        return format_;
    }
 
    RKCameraHal(std::shared_ptr<CamHwItf> dev, int index, int type);
    ~RKCameraHal();
    void init(const uint32_t width, const uint32_t height, const int fps);
    void start(const uint32_t num, std::shared_ptr<RKCameraBufferAllocator> ptr_allocator);
    void stop(void);

	std::shared_ptr<CamHwItf::PathBase>& mpath(void) {
        return mpath_;
    }
	
	std::shared_ptr<CamHwItf> camdev;
    std::shared_ptr<CamHwItf::PathBase> mpath_;
	int type_;
	int index_;
    frm_info_t format_;
};

#endif // __CAMERA_HAL_H__