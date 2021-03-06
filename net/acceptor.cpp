// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "acceptor.h"
#include "base/logging.h"
#include "socket.h"
#include "eventloop.h"
#include "tcpconnection.h"

namespace small_rpc {

// Acceptor
Acceptor::Acceptor(EventLoop* el, const char* addr, unsigned short port) {
    _fd = listen_socket(addr, port);
    _event = EPOLLIN;
    _el = el;
    _el->update_channel(static_cast<Channel*>(this));
}

// handle_events
void Acceptor::handle_events(int events) {
    if (events & EPOLLIN) {
        handler_new_connection();
    }
}

// handler_new_connection
void Acceptor::handler_new_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int conn = ::accept(_fd, reinterpret_cast<struct sockaddr*>(&client_addr),
        &client_addr_len);
    if (conn == -1) {
        if (errno == EAGAIN // nonblocking
                || errno == EWOULDBLOCK // same as EAGAIN
                || errno == ECONNABORTED // get a RST connection
                || errno == EINTR) { // interrupt by signal
            PLOG_DEBUG << "Acceptor accept -1";
            return ;
        } else {
            PLOG_FATAL << "Acceptor failed to invoke ::accept";
        }
    }
    if (_new_connection_callback) {
        _new_connection_callback(conn);
    }
}

}; // namespace small_rpc
