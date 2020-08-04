// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "channel.h"
#include <sys/epoll.h>
#include <vector>

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
    std::vector<struct epoll_event> _events;
};

}; // namespace small_rpc
