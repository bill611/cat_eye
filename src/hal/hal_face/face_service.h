/*
 * Face service class definition
 *
 * Copyright (C) 2018 Rockchip Electronics Co., Ltd.
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

#ifndef FACE_SERVICE_H_
#define FACE_SERVICE_H_

#include <fstream>
#include <list>

#include <CameraHal/CamHwItf.h>

#include <adk/base/service.h>
#include <adk/base/definition_magic.h>
#include <messenger/message/app.h>
#include <messenger/message/face.h>
#include <messenger/message/system.h>

#include "camera/camera_factory.h"
#include "face/face_camera_buffer.h"
#include "face/face_detector.h"
#include "face/face_evaluator.h"
#include "face/face_recognizer.h"
#include "face/live_detector.h"
#include "ipc/messenger.h"
#include "process/capture_process.h"
#include "process/display_process.h"
#include "process/encoder.h"
#include "process/face_preprocess.h"
#include "process/process_factory.h"

#if USE_UVC
#include "process/uvc_process.h"
#endif

namespace rk {

#define PROCESSOR_LENGTH 64

typedef struct FaceServiceProcessor {
    char name[PROCESSOR_LENGTH];
    std::shared_ptr<StreamPUBase> process_unit;
} FaceServiceProcessor;

typedef struct ParseParameter {
    frm_info_t format;
    int buffer_count;
    cJSON* process_node;
} ParseParameter;

class FaceService :
    public Service, FaceDetectListener, FaceRecognizeListener,
        LiveDetectListener, SystemMessageHandler, BusMessageHandler {
 public:
    FaceService() = delete;
    FaceService(std::string& config_path, bool standalone);
    virtual ~FaceService();

    ADK_DECLARE_SHARED_PTR(FaceService);

    virtual void start(void) override;
    virtual void stop(void) override;

    int connect(std::shared_ptr<CamHwItf::PathBase> mpath, std::shared_ptr<StreamPUBase> next,
                frm_info_t& frmFmt, const uint32_t num, std::shared_ptr<FaceCameraBufferAllocator> allocator);
    int connect(std::shared_ptr<StreamPUBase> pre, std::shared_ptr<StreamPUBase> next,
                frm_info_t &frmFmt, const uint32_t num, std::shared_ptr<FaceCameraBufferAllocator> allocator);

    void disconnect(std::shared_ptr<CamHwItf::PathBase> mpath, std::shared_ptr<StreamPUBase> next);
    void disconnect(std::shared_ptr<StreamPUBase> pre, std::shared_ptr<StreamPUBase> next);

    void ParseConfig(const char* path);
    void SetParameter(const cJSON* camera, ParseParameter& parameter);

    void ProcessRun(Camera::SharedPtr cam, cJSON* process_node);
    void ProcessStop(Camera::SharedPtr cam, cJSON* process_node);

    RK_FRMAE_FORMAT convert(const cJSON* format) const {
        if (strcmp(format->valuestring, "NV12") == 0)
            return HAL_FRMAE_FMT_NV12;
        else if (strcmp(format->valuestring, "SBGGR10") == 0)
            return HAL_FRMAE_FMT_SBGGR10;
        else if (strcmp(format->valuestring, "YUYV") == 0)
            return HAL_FRMAE_FMT_YUYV;
        else
            ASSERT(0);
    }

    std::shared_ptr<StreamPUBase> GetProcess(const char* name) {
        for (FaceServiceProcessor processor : processors_) {
            if (strcmp(processor.name, name) == 0)
                return processor.process_unit;
        }
        return nullptr;
    }

    virtual void OnFaceDetected(FaceArray::SharedPtr face_array) override {
        std::shared_ptr<FaceRecognizer> face_recognizer =
                       dynamic_pointer_cast<FaceRecognizer>(GetProcess("FaceRecognizer"));

        if (face_recognizer.get() &&
            face_array->empty() &&
            face_recognizer->mode() == RegisterMode) {
            face_recognizer->ChangeMode();

            FaceRegisteredMessage msg;
            msg.FillBody(-1, 2);
            if (messenger_)
                messenger_->SendMessageToBus(msg.to_string());
        }

        if (messenger_) {
            FaceDetectedMessage msg;
            msg.FillBody(face_array);
            messenger_->SendMessageToBus(msg.to_string());
        }

        // std::shared_ptr<DisplayProcess> displayer =
        //                dynamic_pointer_cast<DisplayProcess>(GetProcess("DisplayProcess"));
        // displayer->SetFaces(face_array);
    }

    virtual void OnRecognisePrepared(void) override  {
        std::shared_ptr<FaceDetector> face_detector =
                       dynamic_pointer_cast<FaceDetector>(GetProcess("FaceDetector"));

        if (face_detector.get())
            face_detector->pause();
    }

    virtual void OnFaceRecognized(Face& face, const int user_id, bool result) override {
        std::shared_ptr<FaceDetector> face_detector =
                       dynamic_pointer_cast<FaceDetector>(GetProcess("FaceDetector"));
        if (face_detector.get())
            face_detector->resume();

        if (messenger_) {
            FaceRecognizedMessage msg;
            msg.FillBody(user_id, result);
            messenger_->SendMessageToBus(msg.to_string());
        }
    }

    virtual void OnFaceRegistered(Face& face, FaceFeature& feature, int result) override {
        std::shared_ptr<FaceRecognizer> face_recognizer =
                       dynamic_pointer_cast<FaceRecognizer>(GetProcess("FaceRecognizer"));
        std::shared_ptr<FaceDetector> face_detector =
                       dynamic_pointer_cast<FaceDetector>(GetProcess("FaceDetector"));

        int user_id = -1;
        if (face_recognizer.get() && result == 0)
            face_recognizer->AddUser(feature, &user_id);

        if (face_detector.get())
            face_detector->resume();
        if (face_recognizer.get())
            face_recognizer->ChangeMode();

        if (feature.empty())
            result = 2;

        if (messenger_) {
            FaceRegisteredMessage msg;
            msg.FillBody(user_id, result);
            messenger_->SendMessageToBus(msg.to_string());
        }
    }

    virtual void OnLiveDetected(bool living) {}

    virtual void HandleRegisterUserMessage(void) override {
        std::shared_ptr<FaceRecognizer> face_recognizer =
                       dynamic_pointer_cast<FaceRecognizer>(GetProcess("FaceRecognizer"));
        if (face_recognizer.get())
            face_recognizer->ChangeMode();
    }

    virtual void HandleDeleteUserMessage(int user_id) override {
        std::shared_ptr<FaceRecognizer> face_recognizer =
                       dynamic_pointer_cast<FaceRecognizer>(GetProcess("FaceRecognizer"));

        if (face_recognizer.get()) {
            if (user_id == -1) {
                face_recognizer->DatabaseClear();
            } else if (user_id >= 0) {
               face_recognizer->DeleteUser(user_id);
            }
        }

        if (messenger_) {
            AppDeleteUserResponseMessage msg;
            msg.FillBody(user_id, true);
            messenger_->SendMessageToBus(msg.to_string());
        }
    }

    virtual void HandleSystemQueryStatusMessage(void) override {
        // TODO: response real status
        SystemResponseStatusMessage msg;
        msg.FillBody("status", "busy");

        if (messenger_)
            messenger_->SendMessageToSystem(msg.to_string());
    }
	void getFileImage(char *path);

 private:
    cJSON* root_;
    cJSON* uvc_count_;
    ParseParameter isp_parameter_;
    ParseParameter cif_parameter_;
    struct rk_cams_dev_info cam_info;

    FaceServiceMessenger::SharedPtr messenger_;
    CameraFactory cam_factory_;
    ProcessFactory process_factory_;
    std::list<Camera::SharedPtr> cameras_;
    Socket::SharedPtr system_sock_;

    std::list<FaceServiceProcessor> processors_;
    std::shared_ptr<FaceCameraBufferAllocator> allocator_;
	bool run_status;
};

} // namespace rk

#endif // FACE_SERVICE_H_
