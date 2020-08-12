// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "tcpconnection.h"
#include "base/logging.h"
#include "eventloop.h"
#include "socket.h"

namespace small_rpc {

// TCPConnection
TCPConnection::TCPConnection(int conn, EventLoop* el) : Channel(conn),
        _protocol(0), _ctx(nullptr), _status(TCPConnection_NeedRead) {

    set_nonblocking(_fd);
    _event = EPOLLIN;
    _el = el;
    _el->update_channel(static_cast<Channel*>(this));
}

// handle_events
void TCPConnection::handle_events(int events) {
    LOG_DEBUG << "http_connection handle events";
    if (events & EPOLLIN) {
        int n = _rbuf.read_fd(_fd);
        if (n == 0) {
            // 客户端关闭
            _close_callback(this);
            return ;
        } else {
            _data_read_callback(this);
        }
    } else if (events & EPOLLOUT) {
        _wbuf.write_fd(_fd);
        // TODO 覆盖写这边的错误
        if (_wbuf.readable() == 0) {
            _write_complete_callback(this);
        }
    } else {
        LOG_WARNING << "TCPConnection handle_events other events";
    }
}

// close
void TCPConnection::close() {
    _close_callback(this);
}

}; // namespace small_rpc
