/*
 * Messenger class definition
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

#ifndef FACE_IPC_MESSENGER_H_
#define FACE_IPC_MESSENGER_H_

#include <messenger/messenger.h>

#include <adk/base/definition_magic.h>

namespace rk {

class BusMessageHandler {
 public:
    BusMessageHandler() {}
    virtual ~BusMessageHandler() {}

    virtual void HandleRegisterUserMessage(void) = 0;
    virtual void HandleDeleteUserMessage(int user_id) = 0;
};

class FaceServiceMessenger : public Messenger {
 public:
    FaceServiceMessenger()
        : Messenger("face_service", "ipc:///tmp/face_service.ipc") {}
    virtual ~FaceServiceMessenger() {}

    ADK_DECLARE_SHARED_PTR(FaceServiceMessenger);

    virtual void RegisterBusMessageHandler(BusMessageHandler* handler) {
        bus_handler_ = handler;
    }

    virtual int ProcessBusMessage(const char* msg_string) override {
        JsonMessage msg(msg_string);

        if (strcmp(msg.action(), "AppRegisterUser") == 0) {
            if (bus_handler_)
                bus_handler_->HandleRegisterUserMessage();
        } else if (strcmp(msg.action(), "AppDeleteUser") == 0) {
            cJSON* body = msg.body();
            cJSON* user_id = cJSON_GetObjectItemCaseSensitive(body, "user_id");
            if (bus_handler_)
                bus_handler_->HandleDeleteUserMessage(user_id->valuedouble);
        } else if (strcmp(msg.action(), "AppDeleteAllUser") == 0) {
            if (bus_handler_)
                bus_handler_->HandleDeleteUserMessage(-1);
        } else if (strcmp(msg.action(), "AtRegisterUser") == 0) {
            if (bus_handler_)
                bus_handler_->HandleRegisterUserMessage();
        } else if (strcmp(msg.action(), "AtDeleteUser") == 0) {
            cJSON* body = msg.body();
            cJSON* user_id = cJSON_GetObjectItemCaseSensitive(body, "user_id");
            if (bus_handler_)
                bus_handler_->HandleDeleteUserMessage(user_id->valuedouble);
        }

        return 0;
    }
 private:
    BusMessageHandler* bus_handler_;
};

} // namespace rk

#endif // FACE_IPC_MESSENGER_H_