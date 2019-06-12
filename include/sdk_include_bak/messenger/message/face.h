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

#ifndef MESSENGER_MESSAGE_FACE_H_
#define MESSENGER_MESSAGE_FACE_H_

#include <messenger/json_message.h>

#include <adk/face/face.h>
#include <adk/face/face_array.h>

namespace rk {

class FaceDetectedMessage : public JsonMessage {
 public:
    FaceDetectedMessage()
        : JsonMessage("any", "Bus", "FaceDetected") {}
    virtual ~FaceDetectedMessage() {}

    void FillBody(FaceArray::SharedPtr face_array) {
        cJSON* body = cJSON_CreateObject();
        ASSERT(body != nullptr);

        cJSON* faces = cJSON_CreateArray();

        for (int i = 0; i < face_array->size(); i++) {
            Face::SharedPtr face = (*face_array)[i];

            cJSON* face_json = cJSON_CreateObject();
            cJSON_AddNumberToObject(face_json, "top", face->rect().top());
            cJSON_AddNumberToObject(face_json, "left", face->rect().left());
            cJSON_AddNumberToObject(face_json, "bottom", face->rect().bottom());
            cJSON_AddNumberToObject(face_json, "right", face->rect().right());

            cJSON_AddItemToArray(faces, face_json);
        }

        cJSON_AddItemToObject(body, "face_array", faces);

        AddBody(body);
    }
};

class FaceRecognizedMessage : public JsonMessage {
 public:
    FaceRecognizedMessage()
        : JsonMessage("any", "Bus", "FaceRecognized") {}
    virtual ~FaceRecognizedMessage() {}

    void FillBody(int user_id, bool result) {
        cJSON* body = cJSON_CreateObject();
        ASSERT(body != nullptr);

        cJSON_AddNumberToObject(body, "user_id", user_id);
        cJSON_AddBoolToObject(body, "result", result);

        AddBody(body);
    }
};

class FaceRegisteredMessage : public JsonMessage {
 public:
    FaceRegisteredMessage()
        : JsonMessage("any", "Bus", "FaceRegistered") {}
    virtual ~FaceRegisteredMessage() {}

    void FillBody(int user_id, int result) {
        cJSON* body = cJSON_CreateObject();
        ASSERT(body != nullptr);

        cJSON_AddNumberToObject(body, "user_id", user_id);
        cJSON_AddNumberToObject(body, "result", result);

        AddBody(body);
    }
};

} // namespace rk

#endif // MESSENGER_MESSAGE_FACE_H_
