// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "eventloop.h"
#include "logging.h"

namespace small_rpc {

EventLoop::EventLoop() : _epfd(-1), _events(InitialEventSize) {
    _epfd = epoll_create(1);
    PLOG_FATAL_IF(_epfd == -1) << "failed to invoke epoll_create";
}

void EventLoop::update_channel(Channel* channel) {
    if (channel->sevent() != channel->event()) {
        if (channel->sevent() == 0) {
            if (channel->event() != 0) {
                // add
                _epoll_ctl(EPOLL_CTL_ADD, channel);
            }
        } else {
            if (channel->event() == 0) {
                // delete
                _epoll_ctl(EPOLL_CTL_DEL, channel);
            } else {
                // mod
                _epoll_ctl(EPOLL_CTL_MOD, channel);
            }
        }
    }
}

void EventLoop::loop() {
    while (true) {
        int err = epoll_wait(_epfd, &_events[0], _events.size(), 500); // 500 ms
        if (err == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                PLOG_FATAL << "failed to invoke epoll_wait";
            }
        }
        for (int i = 0; i < err; ++i) {
            static_cast<Channel*>(_events[i].data.ptr)->handle_events(_events[i].events);
        }
    }
}

void EventLoop::_epoll_ctl(int op, Channel* channel) {
    struct epoll_event event;
    event.data.ptr = static_cast<void*>(channel);
    event.events = channel->event();

    int err = epoll_ctl(_epfd, op, channel->fd(), &event);
    PLOG_FATAL_IF(err == -1) << "failed to invoke epoll_ctl";

    channel->set_sevent(channel->event());
}

}; // namespace small_rpc
