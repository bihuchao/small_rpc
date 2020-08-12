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
    HTTPContext() : _stage(0), _body_size(0), _http_method(-1) {}
    virtual ~HTTPContext() {}

    std::ostream& print(std::ostream& os) const;

public:
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
    ParseProtocolStatus _parse_request_line(Buffer& rd_buf);
    ParseProtocolStatus _parse_response_line(Buffer& rd_buf);
    ParseProtocolStatus _parse_headers(Buffer& rd_buf);
    ParseProtocolStatus _parse_body(Buffer& rd_buf);

public:
    static const size_t MaxHeadlineSize;
    static const size_t MaxHeaderSize;

private:
    int _stage;
    int _body_size;
    int _http_method;
    BufferView _http_url, _http_version, _request_line;
    std::map<std::string, std::string> _headers;
};

// HTTPProtocol
class HTTPProtocol : public Protocol {
public:
    ~HTTPProtocol() {}
    // server
    // parse_request rd_buf => ctx
    ParseProtocolStatus parse_request(Buffer& rd_buf, Context** ctxx);

    // client
    Context* new_context() { return new HTTPContext(); }

    const char* name() { return "HTTP"; };

public:
    static const size_t HTTPMethodMaxLen = 7;
};

}; // namespace small_rpc
