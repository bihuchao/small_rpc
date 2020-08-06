// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "http.h"
#include "logging.h"
#include <algorithm>

namespace small_rpc {

const size_t HTTPContext::MaxHeadlineSize = 65536;
const size_t HTTPContext::MaxHeaderSize   = 65536;

ParseResult HTTPContext::parse(ReadBuffer& rd_buf) {
    while (true) {
        if (_stage == 0) {
            ParseResult res = _parse_request_line(rd_buf);
            if (res == ParseResult_Success) { ++_stage; }
            else { return res; }
        } else if (_stage == 1) {
            ParseResult res = _parse_headers(rd_buf);
            if (res == ParseResult_Success) { ++_stage; }
            else { return res; }
        } else if (_stage == 2) {
            ParseResult res = _parse_body(rd_buf);
            if (res == ParseResult_Success) { ++_stage; }
            else { return res; }
        } else {
            return ParseResult_Success;
        }
    }
    return ParseResult_Success;
}

// _parse_request_line
ParseResult HTTPContext::_parse_request_line(ReadBuffer& rd_buf) {
    size_t size = std::min(rd_buf.readable(), MaxHeadlineSize);
    if (size == 0) { return ParseResult_No_Enough_Data; }

    // find CRLF
    int n = rd_buf.find_crlf(size);
    if (n == -1) {
        return (size == MaxHeadlineSize) ? \
            ParseResult_Error : ParseResult_No_Enough_Data;
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
    if (!HTTPMethod::_HTTPMethod_IsValid(_http_method)) { return ParseResult_Error; }
    if (*(rd_buf.begin() + offset) != ' ') { return ParseResult_Error; }
    ++offset;

    // http url
    char* tmp = static_cast<char*>(memchr(rd_buf.begin() + offset, ' ', n - offset));
    if (tmp == nullptr || tmp == (rd_buf.begin() + offset) ||
        *(rd_buf.begin() + offset) != '/') { return ParseResult_Error; }
    _http_url = BufferView(rd_buf, rd_buf.begin() + offset,
        tmp - rd_buf.begin() - offset);
    char* tmp2 = static_cast<char*>(memchr(rd_buf.begin() + offset + 1, '/',
        tmp - rd_buf.begin() - offset));
    if (tmp2) {
        _service = BufferView(rd_buf, rd_buf.begin() + offset + 1,
            tmp2 - rd_buf.begin() - offset - 1);
        _method = BufferView(rd_buf, tmp2 + 1, tmp - tmp2 - 1);
    }
    offset = tmp - rd_buf.begin() + 1;

    // http version
    tmp = static_cast<char*>(memchr(rd_buf.begin() + offset, ' ', n - offset));
    if (tmp) { return ParseResult_Error; }
    _http_version = BufferView(rd_buf, rd_buf.begin() + offset, n - offset);

    // request line
    _request_line = BufferView(rd_buf, rd_buf.begin(), n);
    rd_buf.retrieve(n + 2);

    return ParseResult_Success;
}

// std::string trim
static void trim(std::string& str) {
    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
}

// _parse_headers
ParseResult HTTPContext::_parse_headers(ReadBuffer& rd_buf) {
    while (true) {
        size_t size = std::min(rd_buf.readable(), MaxHeaderSize);
        if (size == 0) { return ParseResult_No_Enough_Data; }

        // find CRLF
        int n = rd_buf.find_crlf(size);
        if (n == -1) {
            return (size == MaxHeadlineSize) ? \
                ParseResult_Error : ParseResult_No_Enough_Data;
        }
        if (n > 0) {
            const char* tmp = static_cast<const char*>(
                memchr(rd_buf.begin(), ':', n));
            if (!tmp || tmp == rd_buf.begin()) {
                LOG_WARNING << "failed to parse header " << std::string(rd_buf.begin(), n);
                return ParseResult_Error;
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
            return ParseResult_Success;
        }
    }
    return ParseResult_Success;
}

// _parse_body
ParseResult HTTPContext::_parse_body(ReadBuffer& rd_buf) {
    if (rd_buf.readable() >= _body_size) {
        _body = BufferView(rd_buf, rd_buf.begin(), _body_size);
        return ParseResult_Success;
    }
    return ParseResult_No_Enough_Data;
}

// print
std::ostream& HTTPContext::print(std::ostream& os) const {
    os << "HTTPContext:"
        << "\nstage: " << _stage
        << "\nrequest_line: " << _request_line.str()
        << "\nservice: " << _service.str()
        << "\nmethod: " << _method.str()
        << "\nbody_size: " << _body_size
        << "\nheaders: ";
    for (const auto& header : _headers) {
        os << "\n    " << header.first << ": " << header.second;
    }
    return os;
}

// HTTP
const char* HTTPProtocol::HTTP = "HTTP";

// judge_protocol
bool HTTPProtocol::judge_protocol(ReadBuffer& rd_buf) {
    for (int i = HTTPMethod::_HTTPMethod_MIN; i < HTTPMethod::_HTTPMethod_MAX; ++i) {
        const std::string& method = HTTPMethod::_HTTPMethod_Name(i);
        if (strncmp(rd_buf.begin(), method.data(), method.length()) == 0) {
            return true;
        }
    }
    return false;
}

}; // namespace small_rpc
