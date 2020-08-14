// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "tcpserver.h"
#include "base/logging.h"

namespace small_rpc {

// start
bool TCPServer::start() {
    if (_thread_num > 0) {
        // 启动 sub_reactor 线程池，线程数为 _thread_num
        _sub_els = new EventLoop[_thread_num];
        for (int i = 0; i < _thread_num; ++i) {
            std::thread tmp(&EventLoop::loop, &_sub_els[i]);
            _sub_reactor_threads.push_back(std::move(tmp));
        }
        _next_el = 0;
    }
    // 启动 main_reactor 线程
    _main_reactor_thread = std::thread(&EventLoop::loop, &_el);
    return true;
}

// stop
bool TCPServer::stop() {
    // 停止 sub_reactor 线程池
    if (_sub_els) {
        LOG_NOTICE << "stop sub reactors " << _thread_num;
        for (int i = 0; i < _thread_num; ++i) {
            _sub_els[i].stop();
            _sub_reactor_threads[i].join();
        }
        delete [] _sub_els;
        _sub_els = nullptr;
    }
    // 停止 main_reactor 线程
    LOG_NOTICE << "stop main reactor";
    _el.stop();
    if (_main_reactor_thread.joinable()) {
        _main_reactor_thread.join();
    }
    return true;
}

EventLoop* TCPServer::_get_next_el() {
    LOG_DEBUG << "TCPServer get eventloop index: " << _next_el;
    if (_next_el == -1) {
        return &_el;
    }
    _next_el = (_next_el + 1) % _thread_num;
    return &_sub_els[_next_el];
}

}; // namespace small_rpc
