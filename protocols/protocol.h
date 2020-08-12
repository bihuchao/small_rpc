// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "buffer.h"
#include "protocol.pb.h"
#include <ostream>

namespace small_rpc {

// Context
class Context {
public:
    virtual ~Context() {}
    virtual std::ostream& print(std::ostream& os) const = 0;

public:
    const ConnType& conn_type() const { return _conn_type; }
    const RpcStatus& rpc_status() const { return _rpc_status; }
    const std::string& service() const { return _service; }
    const std::string& method() const { return _method; }
    const BufferView& payload_view() const { return _payload_view; }
    const std::string& payload() const { return _payload; }

    void set_conn_type(const ConnType& conn_type) { _conn_type = conn_type; }
    void set_rpc_status(const RpcStatus& rpc_status) { _rpc_status = rpc_status; }
    void set_service(const std::string& service) { _service = service; }
    void set_method(const std::string& method) { _method = method; }
    void set_payload_view(const BufferView& payload_view) { _payload_view = payload_view; }
    void set_payload(const std::string& payload) { _payload = payload; }

    std::string* mutable_payload() { return &_payload; }

protected:
    ConnType _conn_type;
    RpcStatus _rpc_status;
    std::string _service, _method;
    BufferView _payload_view;
    std::string _payload;
};

inline std::ostream& operator<<(std::ostream& os, const Context& ctx) {
    return ctx.print(os);
}

// Protocol
class Protocol {
public:
    // server
    // parse_request rd_buf => ctx
    virtual ParseProtocolStatus parse_request(Buffer& rd_buf, Context** ctx) = 0;
    // pack_response ctx => wr_buf
    virtual bool pack_response(Buffer& wr_buf, const Context* ctx) = 0;

    // client
    // get_request_context
    virtual Context* new_context() = 0;
    // pack_request ctx => wr_buf
    virtual bool pack_request(Buffer& wr_buf, const Context* ctx) = 0;
    // parse_response rd_buf => ctx
    virtual ParseProtocolStatus parse_response(Buffer& rd_buf, Context* ctx) = 0;

    virtual ~Protocol() {}
    virtual const char* name() = 0;
};

}; // namespace small_rpc
