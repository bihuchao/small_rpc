// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "base/timestamp.h"
#include <queue>
#include <functional>

namespace small_rpc {

// TimerManager
// 非线程安全
class TimerManager {
public:
    // Timer
    class Timer {
    public:
        Timer(const TimeStamp& ts, const std::function<void()>& func)
                : _data(std::make_pair(ts, func)) {}

        // 小顶堆
        bool operator< (const Timer& that) const {
            return !(_data.first < that._data.first);
        }

        // timestamp
        const TimeStamp& timestamp() const {
            return _data.first;
        }

        // func
        std::function<void()>& func() {
            return _data.second;
        }

    private:
        std::pair<small_rpc::TimeStamp, std::function<void()>> _data;
    friend class TimerManager;
    };

public:
    bool empty() const {
        return _queue.empty();
    }

    const Timer& top() const {
        return _queue.top();
    }

    void pop() {
        _queue.pop();
    }

    TimeStamp get_latest_timer() const {
        if (_queue.empty()) {
            return TimeStamp();
        }
        const Timer& tm = _queue.top();
        return tm._data.first;
    }

    // add_timer
    void add_timer(const TimeStamp& ts, const std::function<void()>& func) {
        _queue.emplace(ts, func);
    }

    // 不提供删除定时器的接口，定时器是否有效由用户回调函数自行判断
    // void remove_timer();

private:
    std::priority_queue<Timer> _queue;
};

}; // namespace small_rpc
