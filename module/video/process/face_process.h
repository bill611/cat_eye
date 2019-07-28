/*
 * =============================================================================
 *
 *       Filename:  face_process.h
 *
 *    Description:  人脸识别数据流接口
 *
 *        Version:  1.0
 *        Created:  2019-06-19 11:51:10 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _FACE_PROCESS_H
#define _FACE_PROCESS_H

#include <CameraHal/StrmPUBase.h>

typedef void (*FaceCallbackFunc)(void *data,int size);

#define IAMGE_MAX_W 1280
#define IAMGE_MAX_H 720
#define IMAGE_MAX_DATA (IAMGE_MAX_W * IAMGE_MAX_H * 3 / 2 )


typedef struct _CammerData {
	int get_data_end;
	int type;
	int w,h;
	char data[IMAGE_MAX_DATA];
}CammerData;

class FaceProcess : public StreamPUBase {
 public:
    FaceProcess();
    virtual ~FaceProcess();

    bool processFrame(std::shared_ptr<BufferBase> input,
                            std::shared_ptr<BufferBase> output) override;
	bool start_enc(void) const {
		return start_enc_;
	};
	FaceCallbackFunc faceCallback(void) const {
		return faceCallback_;
	};

    void faceInit(FaceCallbackFunc faceCallback);
    void faceUnInit(void);
 private:
    bool start_enc_;
    FaceCallbackFunc faceCallback_;
};


#endif
