#ifndef __CAMERA_FACTORY_H_
#define __CAMERA_FACTORY_H_

#include <CameraHal/CamHwItf.h>
#include <CameraHal/CamCifDevHwItf.h>
#include <CameraHal/cam_types.h>
#include <CameraHal/IonCameraBuffer.h>

class CameraFactory final {
 public:
    CameraFactory() {}
    virtual ~CameraFactory() {}
	shared_ptr<CamHwItf> GetCamHwItf(struct rk_cams_dev_info* cam_info, const int index) 
	{
		 shared_ptr<CamHwItf> dev;
	
		if (cam_info->cam[index]->type == RK_CAM_ATTACHED_TO_ISP) 
		{
			dev = getCamHwItf(&cam_info->isp_dev);
		} 
		else if (cam_info->cam[index]->type == RK_CAM_ATTACHED_TO_CIF) 
		{
			int cif_index = ((struct rk_cif_dev_info*)(cam_info->cam[index]->dev))->cif_index;
			dev = shared_ptr<CamHwItf>(new CamCifDevHwItf(&(cam_info->cif_devs.cif_devs[cif_index])));
		} 
		else 
		{
			return NULL;
		}
	
		return dev;
	}
};

#endif // __CAMERA_FACTORY_H_
