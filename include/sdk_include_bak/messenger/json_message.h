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

#ifndef MESSENGER_JSON_MESSAGE_H_
#define MESSENGER_JSON_MESSAGE_H_

#include <unistd.h>

#include <cjson/cJSON.h>
#include <messenger/message.h>

#include <adk/utils/assert.h>

namespace rk {

class JsonMessage : public Message {
 public:
    JsonMessage() = delete;
    JsonMessage(const char* json) {
        json_ = cJSON_Parse(json);
        ASSERT(json_ != nullptr);
        string_ = nullptr;
    }
    JsonMessage(const char* to, const char* type, const char* action) {
        string_ = nullptr;

        json_ = cJSON_CreateObject();
        ASSERT(json_ != nullptr);

        cJSON_AddNumberToObject(json_, "from", getpid());
        cJSON_AddStringToObject(json_, "to", to);
        cJSON_AddStringToObject(json_, "type", type);
        cJSON_AddStringToObject(json_, "action", action);
    }

    virtual ~JsonMessage() {
        cJSON_Delete(json_);
        if (string_) {
            free(string_);
            string_ = nullptr;
        }
    }

    int from(void) const {
        cJSON* from = cJSON_GetObjectItemCaseSensitive(json_, "from");
        if (from)
            return from->valuedouble;
        else
            return -1;
    }

    const char* to(void) const {
        cJSON* to = cJSON_GetObjectItemCaseSensitive(json_, "to");
        if (cJSON_IsString(to) && (to->valuestring != nullptr))
            return to->valuestring;
        else
            return nullptr;
    }

    const char* type(void) const {
        cJSON* type = cJSON_GetObjectItemCaseSensitive(json_, "type");
        if (cJSON_IsString(type) && (type->valuestring != nullptr))
            return type->valuestring;
        else
            return nullptr;
    }

    const char* action(void) const {
        cJSON* action = cJSON_GetObjectItemCaseSensitive(json_, "action");
        if (cJSON_IsString(action) && (action->valuestring != nullptr))
            return action->valuestring;
        else
            return nullptr;
    }

    cJSON* body(void) {
        return cJSON_GetObjectItemCaseSensitive(json_, "body");
    }

    virtual void AddBody(cJSON* body) {
        cJSON_AddItemToObject(json_, "body", body);
    }

    virtual const char* to_string(void) override {
        if (string_ == nullptr)
            string_ = cJSON_Print(json_);

        return string_;
    }

 private:
    cJSON* json_;
};

} // namespace rk

#endif // MESSENGER_JSON_MESSAGE_H_
