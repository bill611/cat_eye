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

#ifndef MESSENGER_MESSENGER_H_
#define MESSENGER_MESSENGER_H_

#include <unistd.h>

#include <messenger/ipc.h>

#include <adk/base/definition_magic.h>
#include <adk/base/thread.h>
#include <adk/utils/logger.h>

namespace rk {

class SystemMessageHandler {
 public:
    SystemMessageHandler() {}
    virtual ~SystemMessageHandler() {}

    virtual void HandleSystemQueryStatusMessage(void) = 0;
};

class Messenger {
 public:
    Messenger() = delete;
    Messenger(const char* name, const char* url);
    virtual ~Messenger();

    ADK_DECLARE_SHARED_PTR(Messenger);

    virtual int SendMessageToBus(std::string& msg) {
        return bus_sock_->send(msg);
    }
    virtual int SendMessageToBus(const char* msg) {
        return bus_sock_->send(msg);
    }
    virtual int SendMessageToSystem(std::string& msg) {
        return system_sock_->send(msg);
    }
    virtual int SendMessageToSystem(const char* msg) {
        return system_sock_->send(msg);
    }

    void RegisterSystemMessageHandler(SystemMessageHandler* handler) {
        system_handler_ = handler;
    }

    virtual int ReceiveSystemMessage(void) {
        std::string message;
        int bytes = system_sock_->receive(message);
        if (bytes < 0) {
            // TODO
            return bytes;
        }
        ProcessSystemMessage(message.c_str());

        return 0;
    }

    virtual int ReceiveBusMessage(void) {
        std::string message;
        int bytes = bus_sock_->receive(message);
        if (bytes < 0) {
            // TODO
            return bytes;
        }
        ProcessBusMessage(message.c_str());

        return 0;
    }

    virtual int ProcessSystemMessage(const char* msg);
    virtual int ProcessBusMessage(const char* msg) = 0;

    ThreadStatus system_messenger_status(void) const {
        return system_messenger_->status();
    }

    ThreadStatus bus_messenger_status(void) const {
        return bus_messenger_->status();
    }

    std::string& name() {
        return name_;
    }

    std::string& bind_url(void) {
        return bind_url_;
    }

 protected:
    std::string name_;
    std::string bind_url_;
    Socket::SharedPtr system_sock_;
    Socket::SharedPtr bus_sock_;
    Thread::SharedPtr system_messenger_;
    Thread::SharedPtr bus_messenger_;
    SystemMessageHandler* system_handler_;
};

} // namespace rk

#endif // MESSENGER_MESSENGER_H_