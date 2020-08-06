// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "channel.h"
#include "buffer.h"
#include "protocol.h"
#include "callbacks.h"
#include "connection.pb.h"

namespace small_rpc {

class EventLoop;

class TCPConnection : public Channel {
public:
    TCPConnection(int conn, EventLoop* el);

    virtual ~TCPConnection() {}

    void handle_events(int events);

    void set_context(Context* context) { _context = context; }
    Context* context() { return _context; }

    void set_data_read_callback(const DataReadCallback& data_read_callback) {
        _data_read_callback = data_read_callback;
    }
    void set_request_callback(const DataReadCallback& request_callback) {
        _request_callback = request_callback;
    }
    void set_write_complete_callback(const DataReadCallback& write_complete_callback) {
        _write_complete_callback = write_complete_callback;
    }

    TCPConnectionStatus status() { return _status; }

    void set_status(const TCPConnectionStatus& status) { _status = status; }

public:
    static const size_t InitialBufferSize = 10240;

private:
    DataReadCallback _data_read_callback;
    RequestCallback _request_callback;
    WriteCompleteCallback _write_complete_callback;

    size_t _protocol;
    Context* _context;

    ReadBuffer _rbuf;
    WriteBuffer _wbuf;

    TCPConnectionStatus _status;

// TODO move friend class
friend class Server;
};

}; // namespace small_rpc
