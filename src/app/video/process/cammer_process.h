/*
 * =============================================================================
 *
 *       Filename:  cammer_process.h
 *
 *    Description:  摄像头数据流接口
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
#ifndef _CAMMER_PROCESS_H
#define _CAMMER_PROCESS_H

#include <CameraHal/StrmPUBase.h>
#include "queue.h"


class CammerProcess : public StreamPUBase {
 public:
    CammerProcess();
    virtual ~CammerProcess();

    bool processFrame(std::shared_ptr<BufferBase> input,
                            std::shared_ptr<BufferBase> output) override;
 private:
	Queue *cammer_queue;
};


#endif
