// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "tcpconnection.h"
#include "logging.h"
#include "eventloop.h"
#include "socket.h"
// TODO: remove this
#include <unistd.h>

namespace small_rpc {

TCPConnection::TCPConnection(int conn, EventLoop* el) : Channel(conn),
        _protocol(0), _context(nullptr), _status(TCPConnection_NeedRead) {

    set_nonblocking(_fd);
    _event = EPOLLIN;
    _el = el;
    _el->update_channel(static_cast<Channel*>(this));
}

void TCPConnection::handle_events(int events) {
    LOG_DEBUG << "http_connection handle events";
    if (events & EPOLLIN) {
        int n = _rbuf.read_fd(_fd);
        if (n == 0) {
            // 客户端关闭
            LOG_DEBUG << "close client";
            _event = 0;
            _el->update_channel(static_cast<Channel*>(this));
            close(_fd);
            return ;
        }
        _data_read_callback(this);
        if (_status == TCPConnection_Error) {
            // 消息解析失败
            _event = 0;
            _el->update_channel(this);
            close(_fd);
            delete this;
            return ;
        }
        if (_status == TCPConnection_NeedRead) { return ; }
        if (_status == TCPConnection_ReadSuccess) {
            _event = 0;
            _el->update_channel(static_cast<Channel*>(this));
            _request_callback(this);
        }
    } else if (events & EPOLLOUT) {
        _wbuf.write_fd(_fd);
        if (_wbuf.readable() == 0) {
            _write_complete_callback(this);
        }
    } else {
        LOG_WARNING << "other events";
    }
}

}; // namespace small_rpc
