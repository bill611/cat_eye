/*
 * Copyright (C) 2019 Rockchip Electronics Co., Ltd.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL), available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef PROCESS_FACTORY_H_
#define PROCESS_FACTORY_H_

#if USE_UVC
#include "process/uvc_process.h"
#endif

namespace rk {

typedef struct FactoryParameter {
    int width;
    int height;
} FactoryParameter;

class ProcessFactory final {
 public:
    ProcessFactory() {}
    virtual ~ProcessFactory() {}

    std::shared_ptr<StreamPUBase> CreateProcess(const char* name, FactoryParameter& parameter) {
        if (strcmp(name, "DisplayProcess") == 0) {
            DisplayProcess* display = new DisplayProcess();
            return std::shared_ptr<StreamPUBase>(display);
        } else if (strcmp(name, "FacePreprocess") == 0) {
            FacePreprocess* pre_process = new FacePreprocess(parameter.width, parameter.height);
            return std::shared_ptr<StreamPUBase>(pre_process);
        } else if (strcmp(name, "FaceDetector") == 0) {
            FaceDetector* detector = new FaceDetector(kRockchipFaceDetect);
            return std::shared_ptr<StreamPUBase>(detector);
        } else if (strcmp(name, "FaceEvaluator") == 0) {
            FaceEvaluator* evaluator = new FaceEvaluator();
            return std::shared_ptr<StreamPUBase>(evaluator);
        } else if (strcmp(name, "FaceRecognizer") == 0) {
            FaceRecognizer* recognizer =
                new FaceRecognizer(kRockchipFaceRecognition, parameter.width, parameter.height);
            return std::shared_ptr<StreamPUBase>(recognizer);
        } else if (strcmp(name, "LiveDetector") == 0) {
            LiveDetector* live = new LiveDetector(kRockchipLiveDetection);
            return std::shared_ptr<StreamPUBase>(live);
        } else if (strcmp(name, "UvcProcess0") == 0) {
#if USE_UVC
            UvcProcess* uvc = new UvcProcess(0);
            return std::shared_ptr<StreamPUBase>(uvc);
#endif
        } else if (strcmp(name, "UvcProcess1") == 0) {
#if USE_UVC
            UvcProcess* uvc = new UvcProcess(1);
            return std::shared_ptr<StreamPUBase>(uvc);
#endif
        } else {
            printf("CreateProcess %s error\n", name);
            ASSERT(0);
        }
    }

    void DestoryProcess(std::shared_ptr<StreamPUBase> process) {
        process.reset();
        process = nullptr;
    }
};

} // namespace rk

#endif // PROCESS_FACTORY_H_
