// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "channel.h"
#include "timer_manager.h"
#include <sys/epoll.h>
#include <map>
#include <vector>
#include <queue>
#include <atomic>
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

class Channel;

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

    // run_after
    void run_after(int after_time_ms, const std::function<void()>& func) {
        TimeStamp ts = TimeStamp::after_now_ms(after_time_ms);
        {
            std::unique_lock<std::mutex> lg(_timers_mtx);
            _timers.add_timer(ts, func);
        }
        wakeup();
    }

    void add_channel(Channel* channel, const TimeStamp& ts) {
        std::unique_lock<std::mutex> lg(_channels_mtx);
        _channels[channel] = ts;
    }

    void remove_channel(Channel* channel, const TimeStamp& ts) {
        std::unique_lock<std::mutex> lg(_channels_mtx);
        auto it = _channels.find(channel);
        if (it != _channels.end() && it->second == ts) {
            _channels.erase(it);
        }
    }

    bool has_channel(Channel* channel, const TimeStamp& ts) {
        std::unique_lock<std::mutex> lg(_channels_mtx);
        auto it = _channels.find(channel);
        if (it != _channels.end() && it->second == ts) {
            return true;
        }
        return false;
    }

public:
    static const size_t InitialEventSize = 1024;
    static const int DefaultEpollTimeout = 256; // ms

private:
    int _epfd;
    WakeUper _wakeuper;
    std::vector<struct epoll_event> _events;
    std::atomic<bool> _is_stop;

    // 异步调用更改EventLoop状态
    std::vector<std::function<void()>> _funcs;
    std::mutex _funcs_mtx;

    // TimerManager
    TimerManager _timers;
    std::mutex _timers_mtx;

    // Channels
    std::map<Channel*, TimeStamp> _channels;
    std::mutex _channels_mtx;
};

}; // namespace small_rpc
