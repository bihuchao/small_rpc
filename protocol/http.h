// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "protocol.h"
#include <string>
#include <map>

namespace small_rpc {

// HTTPContext
class HTTPContext : public Context {
public:
    HTTPContext() : _stage(0), _body_size(0), _http_method(-1), _conn_type(ConnType_Short) {}

    virtual ~HTTPContext() {}

    ParseResult parse(ReadBuffer& rd_buf);

    std::ostream& print(std::ostream& os) const;

    const BufferView& payload() const { return _body; }

    ConnType conn_type() const { return _conn_type; };

    const BufferView& service() const { return _service; }
    const BufferView& method() const { return _method; }

private:
    ParseResult _parse_request_line(ReadBuffer& rd_buf);
    ParseResult _parse_headers(ReadBuffer& rd_buf);
    ParseResult _parse_body(ReadBuffer& rd_buf);

public:
    static const size_t MaxHeadlineSize; // 65536
    static const size_t MaxHeaderSize; // 65536

private:
    int _stage;
    size_t _body_size;
    int _http_method;
    ConnType _conn_type;
    BufferView _http_url, _http_version, _request_line, _service, _method;
    BufferView _body;
    std::map<std::string, std::string> _headers;
};

// HTTPProtocol
class HTTPProtocol : public Protocol {
public:
    ~HTTPProtocol() {}

    size_t judge_protocol_size() { return HTTPMethodMaxLen; }

    bool judge_protocol(ReadBuffer& rd_buf);

    Context* new_context() {
        return new HTTPContext();
    }

    const char* name() { return HTTP; }
public:
    static const char* HTTP;
    static const size_t HTTPMethodMaxLen = 7;
};

}; // namespace small_rpc
