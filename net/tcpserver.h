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
            : _acceptor(&_el, addr, port), _thread_num(0), _next_el(-1), _sub_els(nullptr) {}

    ~TCPServer() {}

    const int& thread_num() const { return _thread_num; }

    void set_thread_num(const int& thread_num) {
        _thread_num = thread_num;
        if (_thread_num > MaxThreadNum) {
            _thread_num = MaxThreadNum;
        }
        if (_thread_num < 0) {
            _thread_num = 0;
        }
    }

    bool start();

    bool stop();

protected:
    EventLoop* _get_next_el();

public:
    static const int MaxThreadNum = 144;

protected:
    EventLoop _el;
    Acceptor _acceptor;
    int _thread_num;
    std::thread _main_reactor_thread;

    // sub reactors
    int _next_el;
    EventLoop* _sub_els;
    std::vector<std::thread> _sub_reactor_threads;
};

}; // namespace small_rpc
