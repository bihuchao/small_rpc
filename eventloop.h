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

namespace small_rpc {

class EventLoop {
public:
    EventLoop();
    void update_channel(Channel* channel);
    void loop();

private:
    void _epoll_ctl(int op, Channel* channel);

public:
    static const size_t InitialEventSize = 1024;

private:
    int _epfd;
    int _eventfd;

    std::atomic<pid_t> _tid;
    std::atomic<bool> _is_stop;

    // for async 调用
    std::queue<Channel*> _que;
    std::mutex _queue_mtx;

    std::vector<struct epoll_event> _events;

    // TODO 实现读写 连接超时
};

}; // namespace small_rpc
