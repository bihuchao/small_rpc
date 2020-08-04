// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "buffer.h"
#include "connection.pb.h"
#include <ostream>

namespace small_rpc {

// Context
class Context {
public:
    virtual ParseResult parse(ReadBuffer& rd_buf) = 0;
    virtual ~Context() {}
    virtual std::ostream& print(std::ostream& os) const = 0;
    virtual const std::string& payload() const = 0;
    virtual ConnType conn_type() const = 0;
    virtual const std::string& service() const = 0;
    virtual const std::string& method() const = 0;
friend std::ostream& operator<<(std::ostream& os, const Context& ctx);
};

inline std::ostream& operator<<(std::ostream& os, const Context& ctx) {
    return ctx.print(os);
}

// Protocol
class Protocol {
public:
    virtual size_t judge_protocol_size() = 0;
    virtual bool judge_protocol(ReadBuffer& rd_buf) = 0;
    virtual Context* new_context() = 0;
    virtual ~Protocol() {}
    virtual const char* name() = 0;
};

}; // namespace small_rpc
