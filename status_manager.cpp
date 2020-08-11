// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "status_manager.h"
#include "logging.h"

namespace small_rpc {

// register_signals
bool StatusManager::register_signals() {
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sa.sa_handler = StatusManager::_handle_signal;
    int err = sigaction(SIGINT, &sa, nullptr);
    PLOG_FATAL_IF(err == -1) << "StatusManager failed to invoke sigaction";
    return true;
}

// wait_for_signal
void StatusManager::wait_for_signal(int timeout_ms) {
    std::unique_lock<std::mutex> ul(_mtx);
    // LOG_DEBUG << "StatusManager start to wait " << timeout_ms << " ms";
    _cv.wait_until(ul, std::chrono::milliseconds(timeout_ms)
        + std::chrono::system_clock::now());
}

}; // namespace small_rpc
