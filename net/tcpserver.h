// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>
#include "eventloop.h"
#include "tcpconnection.h"
#include "acceptor.h"
#include <thread>

namespace small_rpc {

// TCPServer
class TCPServer {
public:
    TCPServer(const char* addr, unsigned short port)
            : _acceptor(&_el, addr, port), _thread_num(0) {}

    ~TCPServer() {}

    EventLoop& el() { return _el; }

    size_t thread_num() const { return _thread_num; }

    void set_thread_num(const size_t& thread_num) {
        _thread_num = thread_num;
        if (_thread_num > MaxThreadNum) {
            _thread_num = MaxThreadNum;
        }
    }

    bool start();

    bool stop();

public:
    static const size_t MaxThreadNum = 144;

protected:
    EventLoop _el;
    Acceptor _acceptor;
    size_t _thread_num;
    std::thread _main_reactor_thread;
};

}; // namespace small_rpc
