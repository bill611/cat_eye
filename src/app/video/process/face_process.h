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
#include "queue.h"


class FaceProcess : public StreamPUBase {
 public:
    FaceProcess();
    virtual ~FaceProcess();

    bool processFrame(std::shared_ptr<BufferBase> input,
                            std::shared_ptr<BufferBase> output) override;
 private:
	Queue *cammer_queue;
};


#endif
