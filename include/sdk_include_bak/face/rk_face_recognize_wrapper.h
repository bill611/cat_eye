
#ifndef __RK_FACE_RECOGNIZE_WRAPPER_H_

#define __RK_FACE_RECOGNIZE_WRAPPER_H_

#include "common.h"

__attribute__((visibility("default"))) int rkFaceInit(void* buf_virt, void* buf_phys,void* buf_phys_model,const char *license);

//feature : feature buffer pointer
//img_orig : original image data,NV12
//face : face landmark ,locate ,score...
__attribute__((visibility("default"))) int rkFaceFeature( float* feature,RkImage* img_orig,struct RkFace *face);

//feature1
//feature2
//len: feature dimension
//return:Face feature similarity value
__attribute__((visibility("default"))) float rkFeatureIdentify(float* feature1,float* feature2,int len);

//distance: Feature distance buffer,the buffer size must be equaled to "person_num * enroll_feature_num"
//person_num : Number of person
//enroll_feature_num:Number of face feature per person
//return:Recognize result,the index of person
__attribute__((visibility("default"))) int rkRecognizeStrategy(float *distance, int person_num,int enroll_feature_num);


#endif
