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
            ParseResult res = _parse_headline(rd_buf);
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

// _parse_headline
ParseResult HTTPContext::_parse_headline(ReadBuffer& rd_buf) {
    size_t size = std::min(rd_buf.readable(), MaxHeadlineSize);
    if (size == 0) { return ParseResult_No_Enough_Data; }

    // find CRLF
    int n = rd_buf.find_crlf(size);
    if (n == -1) {
        return (size == MaxHeadlineSize) ? \
            ParseResult_Error : ParseResult_No_Enough_Data;
    }

    _headline = std::string(rd_buf.begin(), n);
    rd_buf.retrieve(n + 2);

    return ParseResult_Success;
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
            if (!tmp) {
                LOG_WARNING << "failed to parse header " << std::string(rd_buf.begin(), n);
                return ParseResult_Error;
            }
            // TODO : 判断冒号在行末尾
            //        headers合法性判断
            _headers[std::string(rd_buf.begin(), tmp - rd_buf.begin())] =
                std::string(tmp + 1,  rd_buf.begin() - tmp + n - 1);
            if (tmp - rd_buf.begin() == 14 &&
                    strncmp("Content-Length", rd_buf.begin(), 14) == 0) {
                std::string temp(rd_buf.begin() + 14, rd_buf.begin() + n);
                _body_size = atoi(temp.c_str());
                LOG_NOTICE << "_body_size = " << _body_size;
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
        _body = std::string(rd_buf.begin(), _body_size);
        return ParseResult_Success;
    }
    return ParseResult_No_Enough_Data;
}

std::ostream& HTTPContext::print(std::ostream& os) const {
    os << "HTTPContext, stage: " << _stage << ", body_size: " << _body_size;
    return os;
}

const char* HTTPProtocol::HTTP = "HTTP";

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
