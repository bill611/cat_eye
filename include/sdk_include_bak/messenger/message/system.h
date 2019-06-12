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

#ifndef MESSENGER_MESSAGE_SYSTEM_H_
#define MESSENGER_MESSAGE_SYSTEM_H_

#include <string>
#include <vector>

#include <messenger/json_message.h>

namespace rk {

class SystemQueryReadyMessage : public JsonMessage {
 public:
    SystemQueryReadyMessage()
        : JsonMessage("any", "Survey", "SystemQueryReady") {}
    virtual ~SystemQueryReadyMessage() {}
};

class SystemResponseReadyMessage : public JsonMessage {
 public:
    SystemResponseReadyMessage()
        : JsonMessage("any", "Survey", "SystemResponseReady") {}
    virtual ~SystemResponseReadyMessage() {}

    void FillBody(const char* name, const char* url) {
        cJSON* body = cJSON_CreateObject();
        ASSERT(body != nullptr);

        cJSON_AddStringToObject(body, "name", name);
        cJSON_AddStringToObject(body, "bind_url", url);

        AddBody(body);
    }
};

class SystemQueryStatusMessage : public JsonMessage {
 public:
    SystemQueryStatusMessage()
        : JsonMessage("any", "Survey", "SystemQueryStatus") {}
    virtual ~SystemQueryStatusMessage() {}
};

class SystemResponseStatusMessage : public JsonMessage {
 public:
    SystemResponseStatusMessage()
        : JsonMessage("any", "Survey", "SystemResponseStatus") {}
    virtual ~SystemResponseStatusMessage() {}

    void FillBody(const char* name, const char* status) {
        cJSON* body = cJSON_CreateObject();
        ASSERT(body != nullptr);

        cJSON_AddStringToObject(body, "name", name);
        cJSON_AddStringToObject(body, "status", status);

        AddBody(body);
    }
};

class SystemRequestBusConnectionMessage : public JsonMessage {
 public:
    SystemRequestBusConnectionMessage()
        : JsonMessage("any", "Survey", "SystemRequestBusConnection") {}
    virtual ~SystemRequestBusConnectionMessage() {}

    void FillBody(std::vector<std::string>& url_array) {
        cJSON* body = cJSON_CreateObject();
        ASSERT(body != nullptr);

        cJSON* url_array_json = cJSON_CreateArray();

        for (uint i = 0; i < url_array.size(); i++) {
            std::string& url = url_array[i];
            cJSON* url_json = cJSON_CreateObject();
            cJSON_AddStringToObject(url_json, "url", url.c_str());

            cJSON_AddItemToArray(url_array_json, url_json);
        }

        cJSON_AddItemToObject(body, "nodes", url_array_json);

        AddBody(body);
    }
};

} // namespace rk

#endif // MESSENGER_MESSAGE_SYSTEM_H_