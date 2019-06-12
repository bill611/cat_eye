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

#ifndef MESSENGER_MESSAGE_APP_H_
#define MESSENGER_MESSAGE_APP_H_

#include <messenger/json_message.h>

namespace rk {

class AppRegisterUserMessage : public JsonMessage {
 public:
    AppRegisterUserMessage()
        : JsonMessage("any", "Bus", "AppRegisterUser") {}
    virtual ~AppRegisterUserMessage() {}
};

class AppDeleteUserMessage : public JsonMessage {
 public:
    AppDeleteUserMessage()
        : JsonMessage("any", "Bus", "AppDeleteUser") {}
    virtual ~AppDeleteUserMessage() {}

    void FillBody(int user_id) {
        cJSON* body = cJSON_CreateObject();
        ASSERT(body != nullptr);

        cJSON_AddNumberToObject(body, "user_id", user_id);

        AddBody(body);
    }
};

class AppDeleteUserResponseMessage : public JsonMessage {
 public:
    AppDeleteUserResponseMessage()
        : JsonMessage("any", "Bus", "AppDeleteUserResponse") {}
    virtual ~AppDeleteUserResponseMessage() {}

    void FillBody(int user_id, bool result) {
        cJSON* body = cJSON_CreateObject();
        ASSERT(body != nullptr);

        cJSON_AddNumberToObject(body, "user_id", user_id);
        cJSON_AddBoolToObject(body, "result", result);

        AddBody(body);
    }
};

class AppDeleteAllUserMessage : public JsonMessage {
 public:
    AppDeleteAllUserMessage()
        : JsonMessage("any", "Bus", "AppDeleteAllUser") {}
    virtual ~AppDeleteAllUserMessage() {}
};

} // namespace rk

#endif // MESSENGER_MESSAGE_APP_H_
