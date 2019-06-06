/*
 * Face service class
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

#define TAG "face_service"

#include <adk/utils/assert.h>
#include <adk/utils/logger.h>

#include "face_service.h"
#include "process/display_process.h"

using namespace rk;

void FaceService::SetParameter(const cJSON* camera, ParseParameter& parameter)
{
    cJSON* width = cJSON_GetObjectItemCaseSensitive(camera, "width");
    cJSON* height = cJSON_GetObjectItemCaseSensitive(camera, "height");
    cJSON* fps = cJSON_GetObjectItemCaseSensitive(camera, "fps");
    cJSON* format = cJSON_GetObjectItemCaseSensitive(camera, "format");
    cJSON* buffer_count = cJSON_GetObjectItemCaseSensitive(camera, "buffer_count");
    parameter.process_node = cJSON_GetObjectItemCaseSensitive(camera, "ProcessUnit");

    parameter.format.frmSize.width = width->valueint;
    parameter.format.frmSize.height = height->valueint;
    parameter.format.fps = fps->valueint;
    parameter.format.frmFmt = convert(format);
    parameter.format.colorSpace = HAL_COLORSPACE_JPEG;
    parameter.buffer_count = buffer_count->valueint;

    cJSON* process = nullptr;
    cJSON_ArrayForEach(process, parameter.process_node) {
        cJSON* name = cJSON_GetObjectItemCaseSensitive(process, "name");
        cJSON* width = cJSON_GetObjectItemCaseSensitive(process, "width");
        cJSON* height = cJSON_GetObjectItemCaseSensitive(process, "height");

        FactoryParameter factory_parameter;
        factory_parameter.width = (width ? width->valueint : 0);
        factory_parameter.height = (height ? height->valueint : 0);

        FaceServiceProcessor processor;
        strcpy(processor.name, name->valuestring);

        if (uvc_count_ && uvc_count_->valueint == 0 && strstr(processor.name, "UvcProcess"))
            continue;

        if (uvc_count_ && uvc_count_->valueint == 1 && strcmp(processor.name, "UvcProcess1") == 0)
            continue;

        processor.process_unit = process_factory_.CreateProcess(processor.name, factory_parameter);
        processors_.push_back(processor);
    }
}

void FaceService::ParseConfig(const char* path)
{
    std::ifstream config_file(path);

    config_file.seekg(0, config_file.end);
    size_t file_size  = config_file.tellg();
    config_file.seekg(0, config_file.beg);

    // Read stream from config file
    char* config_stream = (char*)malloc(file_size);
    ASSERT(config_stream != nullptr);
    config_file.read(config_stream, file_size);

    root_ = cJSON_Parse(config_stream);
    if (!root_) {
        const char *error = cJSON_GetErrorPtr();
        pr_err("cJSON_Parse Error, before: %s\n", error);
        ASSERT(0);
    }

#if USE_UVC
    cJSON* uvc = cJSON_GetObjectItemCaseSensitive(root_, "uvc");
    if (uvc) {
        uvc_count_ = cJSON_GetObjectItemCaseSensitive(uvc, "count");
        cJSON* uvc_device = cJSON_GetObjectItemCaseSensitive(uvc, "device");
        if (uvc_count_ && uvc_count_->valueint > 0 && uvc_device)
            android_usb_config_uvc(uvc_device->valuestring);
    }
#endif

    cJSON* camera = NULL;
    cJSON* cameras = cJSON_GetObjectItemCaseSensitive(root_, "Cameras");
    cJSON_ArrayForEach(camera, cameras) {
        const cJSON* name = cJSON_GetObjectItemCaseSensitive(camera, "name");
        if (strcmp(name->valuestring, "ISP") == 0) {
            SetParameter(camera, isp_parameter_);
        } else if (strcmp(name->valuestring, "CIF") == 0) {
            SetParameter(camera, cif_parameter_);
        } else {
            ASSERT(0);
        }
    }

    for (FaceServiceProcessor processor : processors_) {
        if (strcmp(processor.name, "FaceDetector") == 0) {
            std::shared_ptr<FaceDetector> face_detector_ =
                       dynamic_pointer_cast<FaceDetector>(processor.process_unit);
            face_detector_->RegisterListener(this);
        } else if (strcmp(processor.name, "FaceRecognizer") == 0) {
            std::shared_ptr<FaceRecognizer> face_recognizer_ =
                        dynamic_pointer_cast<FaceRecognizer>(processor.process_unit);
            face_recognizer_->RegisterListener(this);
        } else if (strcmp(processor.name, "LiveDetector") == 0) {
            std::shared_ptr<LiveDetector> live_detector_ =
                        dynamic_pointer_cast<LiveDetector>(processor.process_unit);
            live_detector_->RegisterListener(this);
        }
    }

    free(config_stream);
}

void FaceService::ProcessRun(Camera::SharedPtr cam, cJSON* process_node)
{
    cJSON* process = nullptr;
	int i = 0;
    cJSON_ArrayForEach(process, process_node) {
        cJSON* name = cJSON_GetObjectItemCaseSensitive(process, "name");
        cJSON* Parent = cJSON_GetObjectItemCaseSensitive(process, "Parent");
        cJSON* buffer_count = cJSON_GetObjectItemCaseSensitive(process, "buffer_count");

        std::shared_ptr<CamHwItf::PathBase> mpath = nullptr;
        std::shared_ptr<StreamPUBase> pre = nullptr;
        std::shared_ptr<StreamPUBase> next = nullptr;
        std::shared_ptr<FaceCameraBufferAllocator> allocator = nullptr;

        for (FaceServiceProcessor processor : processors_) {
            if (strcmp(name->valuestring, processor.name) == 0)
                next = processor.process_unit;
            if (strcmp(Parent->valuestring, processor.name) == 0)
                pre = processor.process_unit;
            else if (strcmp(Parent->valuestring, "mpath") == 0)
                mpath = cam->mpath();
        }

        if (buffer_count->valueint > 0)
            allocator = allocator_;

		std::cout << "[" << __func__ << i++ << "]" << std::endl;
		std::cout << name->valuestring << std::endl;
		std::cout << Parent->valuestring << std::endl;
        if (pre && next)
            connect(pre, next, cam->format(), buffer_count->valueint, allocator);
        if (mpath && next)
            connect(mpath, next, cam->format(), buffer_count->valueint, allocator);
    }
}

void FaceService::ProcessStop(Camera::SharedPtr cam, cJSON* process_node)
{
    for (int i = cJSON_GetArraySize(process_node) - 1; i >= 0; i--) {
        cJSON* item = cJSON_GetArrayItem(process_node, i);
        cJSON* name = cJSON_GetObjectItemCaseSensitive(item, "name");
        cJSON* Parent = cJSON_GetObjectItemCaseSensitive(item, "Parent");

        std::shared_ptr<CamHwItf::PathBase> mpath = nullptr;
        std::shared_ptr<StreamPUBase> pre = nullptr;
        std::shared_ptr<StreamPUBase> next = nullptr;

        for (FaceServiceProcessor processor : processors_) {
            if (strcmp(name->valuestring, processor.name) == 0)
                next = processor.process_unit;
            if (strcmp(Parent->valuestring, processor.name) == 0)
                pre = processor.process_unit;
            if (strcmp(Parent->valuestring, "mpath") == 0)
                mpath = cam->mpath();
        }

        if (pre && next)
            disconnect(pre, next);
        if (mpath && next)
            disconnect(mpath, next);
    }
}

FaceService::FaceService(std::string& config_path, bool standalone)
{
    uvc_count_ = 0;
    memset(&cam_info, 0, sizeof(cam_info));
    CamHwItf::getCameraInfos(&cam_info);
    ASSERT(cam_info.num_camers > 0);

    allocator_ = shared_ptr<FaceCameraBufferAllocator>(new FaceCameraBufferAllocator());
    ASSERT(allocator_.get() != nullptr);

    ParseConfig(config_path.c_str());

    if (!standalone) {
        messenger_ = std::make_shared<FaceServiceMessenger>();
        messenger_->RegisterBusMessageHandler(this);
        messenger_->RegisterSystemMessageHandler(this);
    }
    // Create cameras.
    for (int i = 0; i < cam_info.num_camers; i++) {
        Camera::SharedPtr new_cam = cam_factory_.CreateCamera(&cam_info, i);
        if (new_cam.get() != nullptr)
            cameras_.push_back(new_cam);
    }
}

FaceService::~FaceService()
{
    for (FaceServiceProcessor processor : processors_) {
        memset(processor.name, 0, sizeof(processor.name));
        process_factory_.DestoryProcess(processor.process_unit);
    }

    if (root_)
        cJSON_Delete(root_);

    allocator_.reset();
}

void FaceService::start(void)
{
	if (run_status == true)
		return;
	run_status = true;
    for (Camera::SharedPtr cam : cameras_) {
        if (cam->type() == ISP_CAMERA) {
            cam->init(isp_parameter_.format);
            cam->start(isp_parameter_.buffer_count, allocator_);
            ProcessRun(cam, isp_parameter_.process_node);
        } else if (cam->type() == CIF_CAMERA) {
            cam->init(cif_parameter_.format);
            cam->start(cif_parameter_.buffer_count, allocator_);
            ProcessRun(cam, cif_parameter_.process_node);
        } else {
            ASSERT(0);
        }
    }
}

void FaceService::stop(void)
{
	if (run_status == false)
		return;
	run_status = false;
    for (Camera::SharedPtr cam : cameras_) {
        cJSON* processes = nullptr;
        if (cam->type() == ISP_CAMERA) {
            ProcessStop(cam, isp_parameter_.process_node);
        } else if (cam->type() == CIF_CAMERA) {
            ProcessStop(cam, cif_parameter_.process_node);
        } else {
            pr_err("camera type error.\n");
            ASSERT(0);
        }
        cam->stop();
    }
}

int FaceService::connect(std::shared_ptr<StreamPUBase> pre, std::shared_ptr<StreamPUBase> next,
                         frm_info_t &frmFmt, const uint32_t num, std::shared_ptr<FaceCameraBufferAllocator> allocator)
{
    if (!pre.get() || !next.get()) {
        pr_err("%s: pre is NULL\n", __func__);
		ASSERT(0);
    }
    pre->addBufferNotifier(next.get());
    next->prepare(frmFmt, num, allocator);
    if (!next->start()) {
        pr_err("pre start failed!\n");
		ASSERT(0);
	}
    return 0;
}

int FaceService::connect(std::shared_ptr<CamHwItf::PathBase> mpath, std::shared_ptr<StreamPUBase> next,
                         frm_info_t& frmFmt, const uint32_t num, std::shared_ptr<FaceCameraBufferAllocator> allocator)
{
    if (!mpath.get() || !next.get()) {
        pr_err("%s: mpath is NULL\n", __func__);
		ASSERT(0);
    }

    mpath->addBufferNotifier(next.get());
    next->prepare(frmFmt, num, allocator);
    if (!next->start()) {
        pr_err("mpath start failed!\n");
		ASSERT(0);
	}
    return 0;
}

void FaceService::disconnect(std::shared_ptr<CamHwItf::PathBase> mpath, std::shared_ptr<StreamPUBase> next)
{
    if (!mpath.get() || !next.get())
        return;

    mpath->removeBufferNotifer(next.get());
    next->stop();
    next->releaseBuffers();
}

void FaceService::disconnect(std::shared_ptr<StreamPUBase> pre, std::shared_ptr<StreamPUBase> next)
{
    if (!pre.get() || !next.get())
        return;

    pre->removeBufferNotifer(next.get());
    next->stop();
    next->releaseBuffers();
}
