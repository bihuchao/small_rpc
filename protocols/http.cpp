// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "http.h"
#include "base/logging.h"

namespace small_rpc {

const size_t HTTPContext::MaxHeadlineSize = 65536;
const size_t HTTPContext::MaxHeaderSize = 65536;

std::ostream& HTTPContext::print(std::ostream& os) const {
    os << "HTTPContext:"
        << "\nstage: " << _stage
        << "\nrequest_line: " << _request_line.str()
        << "\nservice: " << _service
        << "\nmethod: " << _method
        << "\nheaders: ";
    for (const auto& header : _headers) {
        os << "\n    " << header.first << ": " << header.second;
    }
    return os;
}

// parse and pack request / response
// parse_request rd_buf => ctx
ParseProtocolStatus HTTPContext::parse_request(Buffer& rd_buf) {
    while (true) {
        if (_stage == 0) {
            ParseProtocolStatus res = _parse_request_line(rd_buf);
            if (res == ParseProtocol_PartSuccess) { ++_stage; }
            else { return res; }
        } else if (_stage == 1) {
            ParseProtocolStatus res = _parse_headers(rd_buf);
            if (res == ParseProtocol_PartSuccess) { ++_stage; }
            else { return res; }
        } else if (_stage == 2) {
            ParseProtocolStatus res = _parse_body(rd_buf);
            if (res == ParseProtocol_PartSuccess) { ++_stage; }
            else { return res; }
        } else {
            return ParseProtocol_Success;
        }
    }
    return ParseProtocol_Success;
}

// _parse_request_line
ParseProtocolStatus HTTPContext::_parse_request_line(Buffer& rd_buf) {
    size_t size = std::min(rd_buf.readable(), MaxHeadlineSize);
    if (size == 0) { return ParseProtocol_NoEnoughData; }

    // find CRLF
    int n = rd_buf.find_crlf(size);
    if (n == -1) {
        return (size == MaxHeadlineSize) ? \
            ParseProtocol_Error : ParseProtocol_NoEnoughData;
    }

    // http method
    int offset = 0;
    for (int i = HTTPMethod::_HTTPMethod_MIN; i < HTTPMethod::_HTTPMethod_MAX; ++i) {
        const std::string& method = HTTPMethod::_HTTPMethod_Name(i);
        if (strncmp(rd_buf.begin(), method.data(), method.length()) == 0) {
            _http_method = HTTPMethod::_HTTPMethod(i);
            offset += method.length();
            break;
        }
    }
    if (!HTTPMethod::_HTTPMethod_IsValid(_http_method)) { return ParseProtocol_Error; }
    if (*(rd_buf.begin() + offset) != ' ') { return ParseProtocol_Error; }
    ++offset;

    // http url
    char* tmp = static_cast<char*>(memchr(rd_buf.begin() + offset, ' ', n - offset));
    if (tmp == nullptr || tmp == (rd_buf.begin() + offset) ||
        *(rd_buf.begin() + offset) != '/') { return ParseProtocol_Error; }
    _http_url = BufferView(rd_buf, rd_buf.begin() + offset,
        tmp - rd_buf.begin() - offset);
    char* tmp2 = static_cast<char*>(memchr(rd_buf.begin() + offset + 1, '/',
        tmp - rd_buf.begin() - offset));
    if (tmp2) {
        _service = std::string(rd_buf.begin() + offset + 1,
            tmp2 - rd_buf.begin() - offset - 1);
        _method = std::string(tmp2 + 1, tmp - tmp2 - 1);
    }
    offset = tmp - rd_buf.begin() + 1;

    // http version
    tmp = static_cast<char*>(memchr(rd_buf.begin() + offset, ' ', n - offset));
    if (tmp) { return ParseProtocol_Error; }
    _http_version = BufferView(rd_buf, rd_buf.begin() + offset, n - offset);

    // request line
    _request_line = BufferView(rd_buf, rd_buf.begin(), n);
    rd_buf.retrieve(n + 2);

    return ParseProtocol_PartSuccess;
}

// _parse_response_line
ParseProtocolStatus HTTPContext::_parse_response_line(Buffer& rd_buf) {
    size_t size = std::min(rd_buf.readable(), MaxHeadlineSize);
    if (size == 0) { return ParseProtocol_NoEnoughData; }

    // find CRLF
    int n = rd_buf.find_crlf(size);
    if (n == -1) {
        return (size == MaxHeadlineSize) ? \
            ParseProtocol_Error : ParseProtocol_NoEnoughData;
    }
    // request line
    _request_line = BufferView(rd_buf, rd_buf.begin(), n);
    rd_buf.retrieve(n + 2);

    return ParseProtocol_PartSuccess;
}

// std::string trim
static void trim(std::string& str) {
    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
}

// _parse_headers
ParseProtocolStatus HTTPContext::_parse_headers(Buffer& rd_buf) {
    while (true) {
        size_t size = std::min(rd_buf.readable(), MaxHeaderSize);
        if (size == 0) { return ParseProtocol_NoEnoughData; }

        // find CRLF
        int n = rd_buf.find_crlf(size);
        if (n == -1) {
            return (size == MaxHeadlineSize) ? \
                ParseProtocol_Error : ParseProtocol_NoEnoughData;
        }
        if (n > 0) {
            const char* tmp = static_cast<const char*>(
                memchr(rd_buf.begin(), ':', n));
            if (!tmp || tmp == rd_buf.begin()) {
                LOG_WARNING << "failed to parse header " << std::string(rd_buf.begin(), n);
                return ParseProtocol_Error;
            }
            std::string key(rd_buf.begin(), tmp - rd_buf.begin());
            std::string value(tmp + 1, rd_buf.begin() - tmp + n - 1);
            trim(key);
            trim(value);
            if (!key.empty() && !value.empty() && _headers.find(key) == _headers.end()) {
                if (key == "Content-Length") {
                    _body_size = std::stoi(value);
                    LOG_NOTICE << "_body_size = " << _body_size;
                }
                if (key == "Connection" && value == "Keep-alive") {
                    _conn_type = ConnType_Single;
                    LOG_NOTICE << "keepalive";
                }
                _headers[key] = std::move(value);
            }
            rd_buf.retrieve(n + 2);
        } else if (n == 0) {
            rd_buf.retrieve(2);
            return ParseProtocol_PartSuccess;
        }
    }
    return ParseProtocol_PartSuccess;
}

// _parse_body
ParseProtocolStatus HTTPContext::_parse_body(Buffer& rd_buf) {
    if (static_cast<int>(rd_buf.readable()) >= _body_size) {
        _payload_view = BufferView(rd_buf, rd_buf.begin(), _body_size);
        return ParseProtocol_PartSuccess;
    }
    return ParseProtocol_NoEnoughData;
}

// pack_response ctx => wr_buf
bool HTTPContext::pack_response(Buffer& wr_buf) const {
    wr_buf.append("HTTP/1.1 200 OK\r\n");
    wr_buf.append("Connection: close\r\n");
    wr_buf.append("Content-Type: application/data\r\n");
    wr_buf.append("Content-Length: ");
    wr_buf.append(std::to_string(_payload.length()));
    wr_buf.append("\r\n\r\n");
    wr_buf.append_extra(std::move(_payload));
    return true;
}

// pack_request ctx => wr_buf
bool HTTPContext::pack_request(Buffer& wr_buf) const {
    wr_buf.append("POST /");
    wr_buf.append(_service);
    wr_buf.append("/");
    wr_buf.append(_method);
    wr_buf.append(" HTTP/1.1\r\n");
    wr_buf.append("Connection: close\r\n");
    wr_buf.append("Content-Type: application/data\r\n");
    // wr_buf.append("Content-Type: application/json");
    wr_buf.append("Content-Length: ");
    wr_buf.append(std::to_string(_payload.length()));
    wr_buf.append("\r\n\r\n");
    wr_buf.append_extra(std::move(_payload));
    return true;
}

// parse_response rd_buf => ctx
ParseProtocolStatus HTTPContext::parse_response(Buffer& rd_buf) {
    while (true) {
        if (_stage == 0) {
            ParseProtocolStatus res = _parse_response_line(rd_buf);
            if (res == ParseProtocol_PartSuccess) { ++_stage; }
            else { return res; }
        } else if (_stage == 1) {
            ParseProtocolStatus res = _parse_headers(rd_buf);
            if (res == ParseProtocol_PartSuccess) { ++_stage; }
            else { return res; }
        } else if (_stage == 2) {
            ParseProtocolStatus res = _parse_body(rd_buf);
            if (res == ParseProtocol_PartSuccess) { ++_stage; }
            else { return res; }
        } else {
            return ParseProtocol_Success;
        }
    }
    return ParseProtocol_Success;
}

// server
// parse_request rd_buf => ctx
ParseProtocolStatus HTTPProtocol::parse_request(Buffer& rd_buf, Context** ctxx) {
    if (!ctxx) { return ParseProtocol_Error; }
    if (*ctxx == nullptr) {
        if (rd_buf.readable() < HTTPMethodMaxLen) { return ParseProtocol_NoEnoughData; }
        for (int i = HTTPMethod::_HTTPMethod_MIN; i < HTTPMethod::_HTTPMethod_MAX; ++i) {
            const std::string& method = HTTPMethod::_HTTPMethod_Name(i);
            if (strncmp(rd_buf.begin(), method.data(), method.length()) == 0) {
                *ctxx = new HTTPContext();
                break;
            }
        }
    }
    if (*ctxx == nullptr) { return ParseProtocol_TryAnotherProtocol; }
    HTTPContext* ctx = dynamic_cast<HTTPContext*>(*ctxx);
    if (!ctx) {
        LOG_WARNING << "failed to dynamic_cast HTTPContext.";
        return ParseProtocol_Error;
    }
    return ctx->parse_request(rd_buf);
}

}; // namespace small_rpc
