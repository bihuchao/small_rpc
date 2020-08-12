// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "tcpserver.h"
#include "logging.h"

namespace small_rpc {

// 另起线程开始run
// start
bool TCPServer::start() {
    if (_thread_num > 0) {
        // 启动 sub_reactor 线程池，线程数为 _thread_num
        // TODO
        LOG_WARNING << "not support multi-thread mode now.";
        return false;
    }
    // 启动 main_reactor 线程
    _main_reactor_thread = std::thread(&EventLoop::loop, &_el);
    return true;
}

// stop
bool TCPServer::stop() {
    // TODO 多线程支持
    // 停止 sub_reactor线程池
    _el.stop();
    if (_main_reactor_thread.joinable()) {
        _main_reactor_thread.join();
    }
    return true;
}

}; // namespace small_rpc
