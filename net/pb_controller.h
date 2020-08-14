// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <google/protobuf/service.h>

namespace small_rpc {

class Context;

// PbController : manipulate PbServer and PbClient settings
class PbController : public ::google::protobuf::RpcController {
public:
    PbController() {}
    virtual ~PbController() {}

    // Client-side
    void Reset() {}

    bool Failed() const { return false; }

    std::string ErrorText() const { return ""; }

    // Server-side
    void SetFailed(const std::string& reason) {}

    // Client-side - not supported
    void StartCancel() {}
    // Server-side - not supported
    bool IsCanceled() const { return false; }

    void NotifyOnCancel(::google::protobuf::Closure* callback) {}

private:
    // TODO
    // Context* _ctx;
};

}; // namespace small_rpc
