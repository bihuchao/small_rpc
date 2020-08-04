// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "http.h"
#include "logging.h"

namespace small_rpc {

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
