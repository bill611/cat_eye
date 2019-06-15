/*
 * FaceRecognizer class definition
 *
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

#ifndef FACE_RECOGNIZER_H_
#define FACE_RECOGNIZER_H_

#include <mutex>
#include <condition_variable>

#include <CameraHal/StrmPUBase.h>

#include <adk/base/definition_magic.h>
#include <adk/base/thread.h>
#include <adk/base/synchronized_list.h>
#include <adk/face/face_array.h>

#include "face/face_db_manager.h"
#include "lib/rk_face_recognize_library.h"

namespace rk {

#define RECOGNITION_THRESHOLD (0.6)

enum FaceRecognizeMode {
    RecognizeMode  = 0,
    RegisterMode,
};

typedef struct RecogniseRequest {
    Face* face;
    Image* image;
} RecogniseRequest;

class FaceRecognizeListener {
 public:
    FaceRecognizeListener() {}
    virtual ~FaceRecognizeListener() {}

    virtual void OnRecognisePrepared(void) = 0;
    virtual void OnFaceRecognized(Face& face, const int user_id, bool result) = 0;
    virtual void OnFaceRegistered(Face& face, FaceFeature& feature, int result) = 0;
};

class FaceRecognizer : public StreamPUBase {
 public:
    FaceRecognizer(FaceRecognizeAlgorithm type, int width, int height);
    virtual ~FaceRecognizer();

    ADK_DECLARE_SHARED_PTR(FaceRecognizer);

    virtual bool processFrame(shared_ptr<BufferBase> inBuf,
                              shared_ptr<BufferBase> outBuf);
    virtual void RegisterListener(FaceRecognizeListener* listener);

    Face* GetFace(Face::SharedPtr face, int width, int height);
    RecogniseRequest* CreateRequest(Face* face, Image* image);
    void DestroyRequest(RecogniseRequest* request);
    void PushRequest(RecogniseRequest* request);
    RecogniseRequest* PopRequest(void);
    void ClearRequest(void);

    int RecognizerUser(FaceFeature& feature);
    void AddUser(FaceFeature& feature, int* user_id);
    void DeleteUser(int user_id);
    void DatabaseClear(void);

    ThreadStatus ProcessorStatus(void) const {
        if (processor_)
            return processor_->status();

        return kThreadUninited;
    }

    void ChangeMode(void) {
        mode_ = (FaceRecognizeMode)(((uint32_t)mode_ + 1) % 2);
    }

    FaceRecognizeMode mode(void) const {
        return mode_;
    }

    FaceDbManager::SharedPtr face_db_manager(void) const {
        return face_db_manager_;
    }

    std::list<FaceRecognizeListener*> listener_list(void) const {
        return listener_list_;
    }

    FaceRecognizeLibrary::SharedPtr face_recognize_lib(void) const {
        return face_recognize_lib_;
    }

	void getFileImage(char *path);
 private:
    int rga_fd_;
    uint32_t total_;
    std::mutex database_mutex_;
    std::mutex request_mutex_;
    Buffer::SharedPtr rga_buffer_;
    Buffer::SharedPtr img_buffer_;
    FaceRecognizeMode mode_;
    Thread::SharedPtr processor_;
    std::condition_variable cond_;
    SynchronizedList<RecogniseRequest*> cache_;
    FaceDbManager::SharedPtr face_db_manager_;
    std::list<FaceRecognizeListener*> listener_list_;
    FaceRecognizeLibrary::SharedPtr face_recognize_lib_;
};

} // namespace rk

#endif // FACE_RECOGNIZER_H_
