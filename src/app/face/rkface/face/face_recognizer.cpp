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

#include <assert.h>
#include <vector>

#include <face/rkFaceDetectWrapper.h>
#include <adk/base/image.h>

#include <rk_rga/rk_rga.h>
#include <adk/base/landmark.h>
#include <adk/mm/cma_allocator.h>

#include "face_recognizer.h"
#include "face_camera_buffer.h"
#include "jpeg_enc_dec.h"

using namespace rk;

void* FaceRecognizeProcessor(void* data)
{
    FaceRecognizer* process = (FaceRecognizer*)data;
    ASSERT(process != nullptr);

    while (kThreadRunning == process->ProcessorStatus()) {
        RecogniseRequest* request = process->PopRequest();
        if (request == nullptr)
            continue;

        process->ClearRequest();

        FaceRecognizeLibrary::SharedPtr face_recognize_lib =
                                        process->face_recognize_lib();

        FaceFeature feature;
        face_recognize_lib->FeatureExtract(*(request->image), *(request->face), feature);

        int user_id = -1;
        int success = -1;
        int index = process->RecognizerUser(feature);

        switch (process->mode()) {
        case RegisterMode: {
            success = (index == -1) ? 0 : 1;
            for(FaceRecognizeListener* listener : process->listener_list())
                listener->OnFaceRegistered(*(request->face), feature, success);
            break;
        }
        case RecognizeMode: {
            if (index >= 0) {
                process->face_db_manager()->DbGetUserByIndex(index, &user_id);
                success = 0;
            }

            for(FaceRecognizeListener* listener : process->listener_list())
                listener->OnFaceRecognized(*(request->face), user_id, success);
            break;
        }
        default:
            ASSERT(0);
        }

        process->DestroyRequest(request);
    }

    return nullptr;
}

void FaceRecognizer::AddUser(FaceFeature& feature, int* user_id)
{
    std::lock_guard<std::mutex> lock(database_mutex_);
    face_db_manager_->DbAddFeature(feature, user_id);
    total_++;
}

void FaceRecognizer::DeleteUser(int user_id) {
    std::lock_guard<std::mutex> lock(database_mutex_);
    face_db_manager_->DbDeleteUser(user_id);
    total_--;
}

void FaceRecognizer::DatabaseClear(void) {
    std::lock_guard<std::mutex> lock(database_mutex_);
    face_db_manager_->DbReset();
    total_ = 0;
}

int FaceRecognizer::RecognizerUser(FaceFeature& feature) {
    std::lock_guard<std::mutex> lock(database_mutex_);

    if (feature.empty()) return -2;

    std::vector<float> result;
    FaceFeature dst_feature;
    for (int i = 0; i < total_; i++) {
        face_db_manager_->DbGetFeatureByIndex(i, dst_feature);

        float similarity = face_recognize_lib_->FeatureCompare(feature, dst_feature);
        if (similarity > 0)
            result.push_back(similarity);
        else
            pr_err("FeatureCompare similarity = %f\n", similarity);

        dst_feature.clear();
    }

    if (result.empty()) return -1;

    uint32_t index = 0;
    float similarity = result[0];
    for (int i = 0; i < result.size(); i++) {
        if (result[i] - similarity < 0) {
            similarity = result[i];
            index = i;
        }
    }

    return (similarity < RECOGNITION_THRESHOLD) ? index : -1;
}

RecogniseRequest* FaceRecognizer::CreateRequest(Face* face, Image* image)
{
    if (!face || !image) {
        pr_err("CreateRequest failed.\n");
        ASSERT(0);
    }

    RecogniseRequest* request = new RecogniseRequest();
    ASSERT(request != nullptr);

    request->face = face;
    request->image = image;
    return request;
}

void FaceRecognizer::DestroyRequest(RecogniseRequest* request)
{
    ASSERT(request != nullptr);
    delete (request->face);
    delete (request->image);
    delete request;
}

void FaceRecognizer::PushRequest(RecogniseRequest* request)
{
    std::lock_guard<std::mutex> lock(request_mutex_);
    ASSERT(request != nullptr);
    cache_.push_back(request);
}

RecogniseRequest* FaceRecognizer::PopRequest(void)
{
    std::unique_lock<std::mutex> lock(request_mutex_);
    if (cache_.empty()) {
        if (cache_.wait_for(lock, 5) == -1)
        return nullptr;
    }

    RecogniseRequest* request = cache_.front();
    cache_.pop_front();
    return request;
}

void FaceRecognizer::ClearRequest(void)
{
    std::unique_lock<std::mutex> lock(request_mutex_);
    while (!cache_.empty()) {
        RecogniseRequest* request = cache_.front();
        cache_.pop_front();
        DestroyRequest(request);
    }
}

FaceRecognizer::FaceRecognizer(FaceRecognizeAlgorithm type, int width, int height)
                : StreamPUBase("FaceRecognizer", true, true)
{
    // mode_ = RegisterMode;
	mode_ = RecognizeMode;

    rga_fd_ = rk_rga_open();
    ASSERT(rga_fd_ >= 0);

    rga_buffer_ = std::shared_ptr<Buffer>(CmaAlloc(width * height * 3 / 2));
    ASSERT(rga_buffer_.get() != nullptr);

    switch (type) {
    case kRockchipFaceRecognition:
        face_recognize_lib_ = std::make_shared<RkFaceRecognizeLibrary>();
        break;
    default:
        assert(0);
    }

    face_db_manager_ = std::make_shared<FaceDbManager>();
    face_db_manager_->SetCurTableName("faceTable");
    face_db_manager_->DbInit();

    total_ = face_db_manager_->DbRecordCount();

    processor_ = std::make_shared<Thread>("FaceRecognizeProcessor",
                                          FaceRecognizeProcessor, this);
}

FaceRecognizer::~FaceRecognizer()
{
    processor_->set_status(kThreadStopping);
    processor_->join();
    processor_.reset();
    face_recognize_lib_.reset();
}

Face* FaceRecognizer::GetFace(Face::SharedPtr face, int width, int height)
{
    int left = face->rect().left(), top = face->rect().top();
    int right = face->rect().right(), bottom = face->rect().bottom();

    LandMarkArray new_landmarks;

    switch (MAIN_APP_PRE_FACE_ROTATE) {
    case 0:
        for (LandMark landmark : face->landmarks().array()) {
            LandMark new_landmark(landmark.x(), landmark.y());
            new_landmarks.push_back(new_landmark);
        }
        break;
    case 90:
        left = height - face->rect().bottom();
        top = face->rect().left();
        right = height - face->rect().top();
        bottom = face->rect().right();

        for (LandMark landmark : face->landmarks().array()) {
            LandMark new_landmark((float)height - landmark.y(), landmark.x());
            new_landmarks.push_back(new_landmark);
        }
        break;
    case 270:
        left = face->rect().top();
        top = width - face->rect().right();
        right = face->rect().bottom();
        bottom = width - face->rect().left();

        for (LandMark landmark : face->landmarks().array()) {
            LandMark new_landmark(landmark.y(), (float)width - landmark.x());
            new_landmarks.push_back(new_landmark);
        }
        break;
    default:
        pr_err("main_app_rotate_angle error.\n");
        ASSERT(0);
    }

    int new_size = (right - left) * (bottom - left);
    Rect new_rect(top, left, bottom, right);

    Face* new_face = new Face(face->id(),
                              new_size,
                              face->sharpness(),
                              new_landmarks,
                              new_rect);
    return new_face;
}

bool FaceRecognizer::processFrame(shared_ptr<BufferBase> inBuf,
                                  shared_ptr<BufferBase> outBuf)
{
    FaceCameraBuffer::SharedPtr buffer =
                    dynamic_pointer_cast<FaceCameraBuffer>(outBuf);
    ASSERT(buffer.get() != nullptr);

    FaceArray::SharedPtr array = buffer->faces();
    if (!array.get() || array->empty())
        return false;

    int32_t src_fd = inBuf->getFd();
    int32_t src_w = inBuf->getWidth();
    int32_t src_h = inBuf->getHeight();
    int32_t src_fmt = RGA_FORMAT_YCBCR_420_SP;

    int32_t src_vir_w = src_w;
    int32_t src_vir_h = src_h;

    int32_t dst_fd = rga_buffer_->fd();
    int32_t dst_fmt = RGA_FORMAT_YCBCR_420_SP;
    int32_t dst_w = (MAIN_APP_PRE_FACE_ROTATE > 0) ? src_h : src_w;
    int32_t dst_h = (MAIN_APP_PRE_FACE_ROTATE > 0) ? src_w : src_h;

    int ret = rk_rga_ionfd_to_ionfd_rotate(rga_fd_,
                                           src_fd, src_w, src_h, src_fmt, src_w, src_h,
                                           dst_fd, dst_w, dst_h, dst_fmt,
                                           MAIN_APP_PRE_FACE_ROTATE);
    ASSERT(ret == 0);

    Image* image = new Image(rga_buffer_->address(),
                             (uint32_t)rga_buffer_->phys_address(),
                             rga_buffer_->fd(), dst_w, dst_h);

    Face* face = GetFace(array->back(), inBuf->getWidth(), inBuf->getHeight());

    RecogniseRequest* request = CreateRequest(face, image);
    if (request)
        PushRequest(request);

    if (cache_.size() > 0) {
        for(FaceRecognizeListener* listener : listener_list_)
            listener->OnRecognisePrepared();
        cache_.signal();
    }

    return true;
}

void FaceRecognizer::RegisterListener(FaceRecognizeListener* listener)
{
    listener_list_.push_front(listener);
}
void FaceRecognizer::getFileImage(char *path,int w,int h)
{
    // img_buffer_ = std::shared_ptr<Buffer>(CmaAlloc(320 * 180 *3 /2 ));
    // ASSERT(img_buffer_.get() != nullptr);
	FILE *img_fd = fopen(path,"rb");
	if (img_fd == NULL) {
		std::cout << "get:" << path << ",fail" << std::endl;
		return;
	}
	int size = w*h*3/2;
	unsigned char *jpeg_buff = (unsigned char *)malloc(size);
	int leng_read = fread(jpeg_buff,w*h*3/2,1,img_fd);
	printf("len:%d\n", leng_read);
	fclose(img_fd);
	int out_len = 0;
	unsigned char *jpeg_buff_out = (unsigned char *)malloc(size);
	unsigned char *p_jpeg_buff = NULL;
	jpegToYuv420sp(jpeg_buff,54996  ,jpeg_buff_out,&out_len);
	// yuv420spToJpeg(jpeg_buff, w,h,&p_jpeg_buff, &out_len);
	img_fd = fopen("320_180.yuv","wb");
	if (img_fd) {
		fwrite(jpeg_buff_out,out_len,1,img_fd);
		fflush(img_fd);
		fclose(img_fd);
	}
	free(p_jpeg_buff);
	free(jpeg_buff);
	free(jpeg_buff_out);
    // Image* image = new Image(img_buffer_->address(),
                             // (uint32_t)img_buffer_->phys_address(),
                             // img_buffer_->fd(), 320, 180);
	// char *p = (char *)image->data().address();
	// for (int i = 0; i < 10; ++i) {
		// printf("%x ",p[320*180*3/2 - i -1] );	
	// }
		// printf("[w:%d,h:%d]\n",image->width(), image->height());
    // rkFaceDetectInfos result;
	// int i;
	// for (i = 0; i < 10; ++i) {
		// int ret = rkFaceDetectWrapper_detect(image->data().address(),(void*)image->data().phys_address(),
				// image->width(), image->height(), &result);
		// std::cout << path << ":" 
			// << "ret:" << ret << ","
			// << result.objectNum << ","
			// << std::endl;
		// if (result.objectNum) {
			// std::cout << result.objects[0].x << ","
				// << result.objects[0].y << ","
				// << result.objects[0].width << ","
				// << result.objects[0].height << ","
				// << std::endl;
			// break;
		// }
	// }
	// if (i == 10)
		// return;

    // float feature_arr[MAX_FACE_FEATURE_SIZE];
	// rkFaceDetectInfo* info = &result.objects[0];
	// int id = info->id;
	// int size = (info->width) * (info->height);
	// float sharpness = info->combined_score;
	// LandMarkArray landmarks;
	// for (int j = 0; j < 5; j++) {
		// LandMark landmark(info->landmarks[j].x, info->landmarks[j].y);
		// landmarks.push_back(landmark);
	// }

	// Rect rect(info->y, info->x, info->y + info->height, info->x + info->width);
	// Face::SharedPtr face = std::make_shared<Face>(id, size, sharpness, landmarks, rect);


    // RkImage rkImage;
    // rkImage.pixels = (char*)image->data().address();
    // rkImage.type = PIXEL_NV12;
    // rkImage.width = image->width();
    // rkImage.height = image->height();

    // struct RkFace rkFace;
    // // rkFace.name = {0};
    // rkFace.id = face->id();
    // rkFace.grade = face->sharpness();
    // rkFace.is_living = 0;

    // rkFace.rect.x = face->rect().left();
    // rkFace.rect.y = face->rect().top();
    // rkFace.rect.width = (face->rect().right() - face->rect().left());
    // rkFace.rect.height = (face->rect().bottom() - face->rect().top());

    // landmarks = face->landmarks();
    // for (int i = 0; i < MAX_FACE_LANDMARK_NUM; i++) {
        // struct RkPoint* point = &rkFace.landmarks[i];
        // point->x = landmarks.array()[i].x();
        // point->y = landmarks.array()[i].y();
    // }
    // int ret = rkFaceFeature(feature_arr, &rkImage, &rkFace);
    // if (ret)
        // printf("rkFaceFeature failed, ret = %d\n", ret);
	// else  {
        // printf("rkFaceFeature ok,ret = %d,id:%d\n", ret,face->id());
	// }
	// delete image;

}
