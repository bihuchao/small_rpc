// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "channel.h"
#include <sys/epoll.h>
#include <vector>
#include <atomic>
#include <queue>
#include <mutex>
#include <functional>

namespace small_rpc {

// WakeUper
class WakeUper : public Channel {
public:
    WakeUper(EventLoop* el);
    ~WakeUper() {}

    void setup();
    void wakeup();
    void handle_events(int events);
};

// EventLoop
class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void stop();
    void wakeup();
    void update_channel(Channel* channel);

private:
    void _epoll_ctl(int op, Channel* channel);

public:
    // 在别的线程中调用 线程安全
    void add_func(const std::function<void()>& func);

public:
    static const size_t InitialEventSize = 1024;

private:
    int _epfd;
    std::vector<struct epoll_event> _events;
    std::atomic<bool> _is_stop;
    WakeUper _wakeuper;

    // 异步调用更改EventLoop状态
    std::vector<std::function<void()>> _funcs;
    std::mutex _funcs_mtx;
    // TODO 实现读写 连接超时
};

}; // namespace small_rpc
