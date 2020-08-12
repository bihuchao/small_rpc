// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <google/protobuf/service.h>

namespace small_rpc {

// DoneGuard
class DoneGuard {
public:
    DoneGuard(::google::protobuf::Closure* done = nullptr) : _done(done) {}
    ~DoneGuard() { _run(); }

    void release() { _done = nullptr; }
    void reset(::google::protobuf::Closure* done) {
        _run();
        _done = done;
    }

private:
    void _run() {
        if (_done) {
            _done->Run();
            _done = nullptr;
        }
    }

protected:
    DoneGuard(const DoneGuard& that) = delete;
    DoneGuard& operator=(const DoneGuard& that) = delete;

private:
    ::google::protobuf::Closure* _done;
};

}; // namespace small_rpc
