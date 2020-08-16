// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include <thread>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "base/status_manager.h"
#include "base/logging.h"
#include "net/eventloop.h"

void func(int timeout_ms, const char* comment) {
    LOG_DEBUG << "in " << comment << " with timeout: " << timeout_ms;
}

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

    el.run_after(500, std::bind(func, 500, "call 1"));
    el.run_after(1000, std::bind(func , 1000, "call 2"));
    el.run_after(200, std::bind(func, 200, "call 3"));
    el.run_after(5000, std::bind(func, 5000, "call 4"));
    el.run_after(3000, std::bind(func, 3000, "call 5"));
    el.run_after(500, std::bind(func, 500, "call 6"));

    while (!sm.is_close()) {
        sm.wait_for_signal();
    }
    LOG_NOTICE << "stop test_eventloop";

    el.stop();
    t.join();
    LOG_DEBUG << "end main";

    return 0;
}
