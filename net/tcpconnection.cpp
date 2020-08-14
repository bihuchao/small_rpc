// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "tcpconnection.h"
#include "base/logging.h"
#include "eventloop.h"
#include "socket.h"

namespace small_rpc {

// handle_events
void TCPConnection::handle_events(int events) {
    LOG_DEBUG << "http_connection handle events";
    if (events & EPOLLIN) {
        int n = _rbuf.read_fd(_fd);
        if (n == 0) {
            // 客户端关闭
            if (_close_callback) {
                _close_callback(this);
            }
            return ;
        } else {
            if (_data_read_callback) {
                _data_read_callback(this);
            }
        }
    } else if (events & EPOLLOUT) {
        if (_connected) {
            _wbuf.write_fd(_fd);
            // TODO 覆盖写这边的错误
            if (_wbuf.readable() == 0) {
                if (_write_complete_callback) {
                    _write_complete_callback(this);
                }
            }
        } else {
            if (_client_conn_callback) {
                _client_conn_callback(this);
            }
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
