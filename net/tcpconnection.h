// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "channel.h"
#include "buffer.h"
#include "base/timestamp.h"
#include "protocols/protocol.h"
#include "callbacks.h"
#include "protocol.pb.h"
#include <atomic>

namespace small_rpc {

class EventLoop;

// TCPConnection
class TCPConnection : public Channel {
public:
    TCPConnection(int conn, EventLoop* el = nullptr, bool connected = true)
            : Channel(conn, el), proto_idx(0), ts(TimeStamp::now()),
              _connected(connected), _ctx(nullptr) {
        timer_id.store(0);
    }

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
    void set_client_conn_callback(const ClientConnCallback& client_conn_callback) {
        _client_conn_callback = client_conn_callback;
    }

    void close();

    const Context* context() { return _ctx; }
    Context** mutable_context() { return &_ctx; }
    void set_context(Context* ctx) { _ctx = ctx; }

    bool connected() { return _connected.load(); }

    Buffer& rbuf() { return _rbuf; }
    Buffer& wbuf() { return _wbuf; }

public:
    static const size_t InitialBufferSize = 10240;

public:
    size_t proto_idx;
    // timestamp
    TimeStamp ts;
    std::atomic<int> timer_id;

protected:
    // status
    std::atomic<bool> _connected;

    // context
    Context* _ctx;

    // callbacks
    ClientConnCallback _client_conn_callback;
    DataReadCallback _data_read_callback;
    DataWriteCompleteCallback _write_complete_callback;
    ConnectionCloseCallback _close_callback;

    Buffer _rbuf, _wbuf;
};

}; // namespace small_rpc
