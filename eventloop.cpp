// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "eventloop.h"
#include "logging.h"
#include <sys/eventfd.h>

namespace small_rpc {

// WakeUper
WakeUper::WakeUper(EventLoop* el) : Channel(-1, el) {
    _fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    PLOG_FATAL_IF(_fd == -1) << "failed to invoke ::eventfd";
    _event = EPOLLIN;
}

// setup
void WakeUper::setup() {
    _event = EPOLLIN;
    _el->update_channel(this);
}

// wakeup
void WakeUper::wakeup() {
    uint64_t wakeup_data = 1;
    int n = ::write(_fd, static_cast<void*>(&wakeup_data), sizeof(wakeup_data));
    if (n == -1) {
        if (errno == EAGAIN || // for O_NONBLOCK IO
                errno == EWOULDBLOCK || // same as EAGAIN
                errno == EINTR) { // sig interrupt
        } else {
            PLOG_FATAL << "failed to invoke ::write";
        }
    }
    LOG_DEBUG << "WakeUper wakeup";
}

// handle_events
void WakeUper::handle_events(int events) {
    if (events & EPOLLIN) {
        uint64_t wakeup_data;
        int n = ::read(_fd, static_cast<void*>(&wakeup_data), sizeof(wakeup_data));
        if (n == -1) {
            if (errno == EAGAIN || // for O_NONBLOCK IO
                    errno == EWOULDBLOCK || // same as EAGAIN
                    errno == EINTR) { // sig interrupt
            } else {
                PLOG_FATAL << "failed to invoke ::read";
            }
        }
        LOG_DEBUG << "WakeUper read " << wakeup_data;
    }
}

// EventLoop
EventLoop::EventLoop() : _epfd(-1), _wakeuper(this), _events(InitialEventSize) {
    _epfd = ::epoll_create(1);
    PLOG_FATAL_IF(_epfd == -1) << "failed to invoke ::epoll_create";
    _wakeuper.setup();
    _is_stop.store(false);
}

// ~EventLoop
EventLoop::~EventLoop() {
    if (_epfd != -1) {
        ::close(_epfd);
        _epfd = -1;
    }
}

// loop
void EventLoop::loop() {
    while (!_is_stop.load()) {
        int err = ::epoll_wait(_epfd, &_events[0], _events.size(), 500); // 500 ms
        // PLOG_DEBUG << "wakeup from epoll_wait, err: " << err;
        if (err == -1) {
            if (errno == EAGAIN || // for O_NONBLOCK IO
                    errno == EWOULDBLOCK || // same as EAGAIN
                    errno == EINTR) { // sig interrupt
                continue;
            } else {
                PLOG_FATAL << "failed to invoke ::epoll_wait";
            }
        }
        for (int i = 0; i < err; ++i) {
            static_cast<Channel*>(_events[i].data.ptr)->handle_events(_events[i].events);
        }
    }
}

// stop
void EventLoop::stop() {
    _is_stop.store(true);
    wakeup();
    LOG_NOTICE << "EventLoop stop";
}

// wakeup
void EventLoop::wakeup() {
    _wakeuper.wakeup();
    LOG_NOTICE << "EventLoop wakeup";
}

// update_channel
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

// _epoll_ctl
void EventLoop::_epoll_ctl(int op, Channel* channel) {
    struct epoll_event event;
    event.data.ptr = static_cast<void*>(channel);
    event.events = channel->event();

    int err = ::epoll_ctl(_epfd, op, channel->fd(), &event);
    PLOG_FATAL_IF(err == -1) << "failed to invoke ::epoll_ctl";

    channel->set_sevent(channel->event());
}

}; // namespace small_rpc
