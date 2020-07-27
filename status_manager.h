// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <condition_variable>
#include <mutex>
#include <chrono>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <iostream>

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
    bool register_signals() {
        struct sigaction sa;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, SIGINT);
        sa.sa_handler = StatusManager::_handle_signal;
        if (sigaction(SIGINT, &sa, nullptr) == -1) {
            printf("StatusManager failed to invoke sigaction, err_no: %d, err_msg : %s\n",
                errno, strerror(errno));
            return false;
        }
        return true;
    }

    void wait_for_signal(int timeout_ms = 500) {
        std::unique_lock<std::mutex> ul(_mtx);
        std::cout << timeout_ms << std::endl;
        _cv.wait_until(ul, std::chrono::milliseconds(timeout_ms) + std::chrono::system_clock::now());
    }

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
