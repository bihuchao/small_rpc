// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "logging.h"
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <atomic>
#include <vector>
#include "eventloop.h"
#include <thread>
#include "status_manager.h"

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, false);
    google::SetCommandLineOption("flagfile", "conf/app.flags");
    std::shared_ptr<void> gflags_guard(nullptr, [](void*){
        LOG_DEBUG << "defer gflags";
        google::ShutDownCommandLineFlags();
    });

    LOG_DEBUG << "in main";
    small_rpc::EventLoop el;
    std::thread t(&small_rpc::EventLoop::loop, &el);

    LOG_NOTICE << "register signal manager.";
    small_rpc::StatusManager& sm = small_rpc::StatusManager::get_instance();
    sm.register_signals();
    while (!sm.is_close()) {
        sm.wait_for_signal();
    }
    LOG_NOTICE << "stop test_eventloop";

    el.stop();
    t.join();
    LOG_DEBUG << "end main";

    return 0;
}
