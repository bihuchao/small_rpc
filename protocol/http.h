// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "protocol.h"

namespace small_rpc {

// HTTPContext
class HTTPContext : public Context {
public:
    HTTPContext() {}
    virtual ~HTTPContext() {}
};

// HTTPProtocol
class HTTPProtocol : public Protocol {
public:
    ~HTTPProtocol() {}

    size_t judge_protocol_size() { return HTTPMethodMaxLen; }

    bool judge_protocol(ReadBuffer& rd_buf);

    Context* new_context() {
        // return new HTTPContext();
        return nullptr;
    }

    const char* name() { return HTTP; }
public:
    static const char* HTTP;
    static const size_t HTTPMethodMaxLen = 7;
};


}; // namespace small_rpc
