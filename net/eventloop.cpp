// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "eventloop.h"
#include "base/logging.h"
#include <sys/eventfd.h>

namespace small_rpc {

// WakeUper
WakeUper::WakeUper(EventLoop* el) : Channel(-1, el) {
    _fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    PLOG_FATAL_IF(_fd == -1) << "WakeUper failed to invoke ::eventfd";
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
                errno == EINTR) { // interrupt by signal
        } else {
            PLOG_FATAL << "WakeUper failed to invoke ::write";
        }
    }
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
                PLOG_FATAL << "WakeUper failed to invoke ::read";
            }
        }
    }
}

// EventLoop
EventLoop::EventLoop() : _epfd(-1), _wakeuper(this), _events(InitialEventSize) {
    _epfd = ::epoll_create(1);
    PLOG_FATAL_IF(_epfd == -1) << "EventLoop failed to invoke ::epoll_create";
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
        int timeout_ms = DefaultEpollTimeout;
        {
            std::unique_lock<std::mutex> lg(_timers_mtx);
            if (!_timers.empty()) {
                TimeStamp now = TimeStamp::now();
                TimeStamp latest = _timers.get_latest_timer();
                timeout_ms = latest - now;
            }
        }
        if (timeout_ms < 0) {
            timeout_ms = 0;
        }
        // LOG_DEBUG << "start to invoke ::epoll_wait, timeout_ms: " << timeout_ms;
        int err = ::epoll_wait(_epfd, &_events[0], _events.size(), timeout_ms);
        // LOG_DEBUG << "wakeup from epoll_wait, err: " << err;
        if (err == -1) {
            if (errno == EAGAIN || // for O_NONBLOCK IO
                    errno == EWOULDBLOCK || // same as EAGAIN
                    errno == EINTR) { // sig interrupt
                continue;
            } else {
                PLOG_FATAL << "EventLoop failed to invoke ::epoll_wait";
            }
        }
        // 最大1024个描述符，暂不考虑扩容
        for (int i = 0; i < err; ++i) {
            static_cast<Channel*>(_events[i].data.ptr)->handle_events(_events[i].events);
        }
        // 执行 _funcs
        std::vector<std::function<void()>> funcs;
        {
            std::unique_lock<std::mutex> lg(_funcs_mtx);
            std::swap(_funcs, funcs);
        }
        for (auto& func : funcs) {
            func();
        }
        // 执行 timers
        TimeStamp now = TimeStamp::now();
        std::vector<TimerManager::Timer> exe_timers;
        {
            std::unique_lock<std::mutex> lg(_timers_mtx);
            while (!_timers.empty()) {
                const TimerManager::Timer& tm = _timers.top();
                if (tm.timestamp() < now) {
                    exe_timers.push_back(tm);
                    _timers.pop();
                } else {
                    break;
                }
            }
        }
        for (auto& tm : exe_timers) {
            tm.func()();
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
    PLOG_FATAL_IF(err == -1) << "EventLoop failed to invoke ::epoll_ctl";

    channel->set_sevent(channel->event());
}

// add_func
void EventLoop::add_func(const std::function<void()>& func) {
    std::unique_lock<std::mutex> lg(_funcs_mtx);
    _funcs.push_back(func);
    wakeup();
}

}; // namespace small_rpc
