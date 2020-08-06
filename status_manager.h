// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <condition_variable>
#include <mutex>
#include <chrono>
#include <signal.h>
#include <errno.h>
#include <string.h>

namespace small_rpc {

class StatusManager {
public:
    static StatusManager& get_instance() {
        static StatusManager sm;
        return sm;
    }

private:
    static void _handle_signal(int signo) {
        StatusManager& sm = StatusManager::get_instance();
        sm._set_close();
    }

public:
    bool register_signals();

    void wait_for_signal(int timeout_ms = 500);

    bool is_close() {
        std::unique_lock<std::mutex> ul(_mtx);
        return _exit;
    }

private:
    void _set_close() {
        std::unique_lock<std::mutex> ul(_mtx);
        _exit = true;
        _cv.notify_all();
    }

protected:
    StatusManager() : _exit(false) {}
    StatusManager(const StatusManager& that) = delete;
    StatusManager& operator=(const StatusManager& that) = delete;

private:
    bool _exit;
    std::mutex _mtx;
    std::condition_variable _cv;
};

}; // namespace small_rpc
