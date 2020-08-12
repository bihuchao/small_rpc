// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "acceptor.h"
#include "logging.h"
#include "socket.h"
#include "eventloop.h"
#include "tcpconnection.h"

namespace small_rpc {

Acceptor::Acceptor(EventLoop* el, const char* addr, unsigned short port) {
    _fd = listen_socket(addr, port);
    _event = EPOLLIN;
    _el = el;
    _el->update_channel(static_cast<Channel*>(this));
}

void Acceptor::handle_events(int events) {
    LOG_DEBUG << "acceptor handle events";
    if (events & EPOLLIN) {
        handler_new_connection();
    }
}

void Acceptor::handler_new_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int conn = accept(_fd, reinterpret_cast<struct sockaddr*>(&client_addr),
        &client_addr_len);
    if (conn == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            LOG_DEBUG << "acceptor accept EAGAIN";
            return ;
        } else {
            PLOG_FATAL << "failed to invoke accept";
        }
    }
    TCPConnection* http_conn = new TCPConnection(conn, _el);
    if (_new_connection_callback) {
        _new_connection_callback(http_conn);
    }
}

}; // namespace small_rpc
