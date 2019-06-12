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

#ifndef MESSENGER_IPC_H_
#define MESSENGER_IPC_H_

#include <string.h>

#include <list>

#include <nanomsg/nn.h>
#include <nanomsg/survey.h>
#include <nanomsg/bus.h>
#include <nanomsg/pair.h>

#include <adk/base/definition_magic.h>
#include <adk/utils/assert.h>
#include <adk/utils/logger.h>

namespace rk {

class Connection {
 public:
    enum class Type { Connect, Bind };

    Connection(int socket_id, std::string const& url, Type type)
        : socket_id_(socket_id)
        , connection_id_(type == Type::Connect ? nn_connect(socket_id_, url.c_str()) : nn_bind(socket_id_, url.c_str()))
    {
        ASSERT(socket_id_ >= 0);
        ASSERT(connection_id_ >= 0);
    }

    ~Connection() {
        nn_shutdown(socket_id_, connection_id_);
    }

    ADK_DECLARE_SHARED_PTR(Connection);

 private:
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    const int socket_id_;
    const int connection_id_;
};

class Socket
{
 public:
    Socket(int type)
        : socket_id_(nn_socket(AF_SP, type)) {
        ASSERT(socket_id_ >= 0);
    }

    ~Socket() {
        while (connections_.size()) {
            Connection::SharedPtr connection = connections_.back();
            connections_.pop_back();
            connection.reset();
        }
        nn_close(socket_id_);
    }

    ADK_DECLARE_SHARED_PTR(Socket);

    void bind(const std::string& url) {
        Connection::SharedPtr connection = std::make_shared<Connection>(socket_id_, url, Connection::Type::Bind);
        connections_.push_back(connection);
    }

    void connect(const std::string& url) {
        Connection::SharedPtr connection = std::make_shared<Connection>(socket_id_, url, Connection::Type::Connect);
        connections_.push_back(connection);
    }

    int send(const std::string& message) {
        ASSERT(connections_.size() > 0);
        uint bytes = nn_send(socket_id_, message.c_str(), message.size(), 0);
        // if (bytes < 0)
        //     pr_err("Send msg failed, errno=%d\n", errno);

        return bytes;
    }

    int send(const char* message) {
        ASSERT(connections_.size() > 0);
        uint bytes = nn_send(socket_id_, message, strlen(message) + 1, 0);
        // if (bytes < 0)
        //    pr_err("Send msg failed, errno=%d\n", errno);

        return bytes;
    }

    int receive(std::string& string) {
        ASSERT(connections_.size() > 0);

        char* buf = nullptr;
        int bytes = nn_recv(socket_id_, &buf, NN_MSG, 0);
        if (bytes > 0) {
            string = buf;
            nn_freemsg(buf);
        } else {
            // pr_err("Receive message errno=%d\n", errno);
        }

        return bytes;
    }

    void set_receive_timeout(int timeout) {
        nn_setsockopt(socket_id_, NN_SOL_SOCKET, NN_RCVTIMEO, &timeout, sizeof(timeout));
    }

    void set_send_timeout(int timeout) {
        nn_setsockopt(socket_id_, NN_SOL_SOCKET, NN_SNDTIMEO, &timeout, sizeof(timeout));
    }

    bool is_connected(void) const {
        if (connections_.size() >= 2)
            return true;
        else
            return false;
    }

    int socket_id(void) {
        return socket_id_;
    }

 private:
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    const int socket_id_;
    std::list<Connection::SharedPtr> connections_;
};

} // namespace rk

#endif // MESSENGER_IPC_H_
