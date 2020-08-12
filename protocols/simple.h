// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "protocol.h"

namespace small_rpc {

// SimpleContext
class SimpleContext : public Context {
public:
    virtual ~SimpleContext() {}
    SimpleContext() : _stage(0) {}
    std::ostream& print(std::ostream& os) const;

    // parse and pack request / response
    // parse_request rd_buf => ctx
    ParseProtocolStatus parse_request(Buffer& rd_buf);
    // pack_response ctx => wr_buf
    bool pack_response(Buffer& wr_buf) const;
    // pack_request ctx => wr_buf
    bool pack_request(Buffer& wr_buf) const;
    // parse_response rd_buf => ctx
    ParseProtocolStatus parse_response(Buffer& rd_buf);
private:
    int _stage;
};

// SimpleProtocol
// magic_num | conn_type | body_len | body
// body => service_len | service | method_len | method | data_len | data
class SimpleProtocol : public Protocol {
public:
    ~SimpleProtocol() {}
    // server
    // parse_request rd_buf => ctx
    ParseProtocolStatus parse_request(Buffer& rd_buf, Context** ctx);

    // client
    Context* new_context() { return new SimpleContext(); }

    const char* name() { return "simple_proto"; };
public:
    static const int MAGIC_NUM = 0x19951028;
};

}; // namespace small_rpc
