// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "channel.h"
#include "buffer.h"
#include "protocol.h"
#include "callbacks.h"
#include "protocol.pb.h"

namespace small_rpc {

class EventLoop;

class TCPConnection : public Channel {
public:
    TCPConnection(int conn, EventLoop* el);

    virtual ~TCPConnection() {}

    void handle_events(int events);

    void set_data_read_callback(const DataReadCallback& data_read_callback) {
        _data_read_callback = data_read_callback;
    }

    void set_write_complete_callback(const DataWriteCompleteCallback& write_complete_callback) {
        _write_complete_callback = write_complete_callback;
    }
    void set_close_callback(const ConnectionCloseCallback& close_callback) {
        _close_callback = close_callback;
    }

    void close();

    TCPConnectionStatus status() { return _status; }

    void set_status(const TCPConnectionStatus& status) { _status = status; }

    const Context* context() { return _ctx; }
    Context** mutable_context() { return &_ctx; }
    void set_context(Context* ctx) { _ctx = ctx; }

public:
    static const size_t InitialBufferSize = 10240;

private:
    // TODO 判断是否为空
    DataReadCallback _data_read_callback;
    DataWriteCompleteCallback _write_complete_callback;
    ConnectionCloseCallback _close_callback;

    Buffer _rbuf, _wbuf;

    TCPConnectionStatus _status;

    size_t _protocol;
    Context* _ctx;

// TODO move friend class
friend class PbServer;
};

}; // namespace small_rpc
