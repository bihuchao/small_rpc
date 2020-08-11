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

DEFINE_int32(wait_ms, 500, "wait ms");

bool is_stop = false;

void deal_sig_int(int signo) {
    LOG_NOTICE << "in deal_sig_int: " << signo;
    is_stop = true;
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, false);
    google::SetCommandLineOption("flagfile", "conf/app.flags");
    std::shared_ptr<void> gflags_guard(nullptr, [](void*){
        LOG_DEBUG << "defer gflags";
        google::ShutDownCommandLineFlags();
    });

    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = deal_sig_int;
    int err = sigaction(SIGINT, &sa, nullptr);
    PLOG_FATAL_IF(err == -1) << "failed to invoke sigaction";

    LOG_DEBUG << "in main";
    small_rpc::EventLoop el;
    std::thread t(&small_rpc::EventLoop::loop, &el);

    while (!is_stop) {
        ::sleep(FLAGS_wait_ms);
    }
    LOG_DEBUG << "stop test_eventloop";

    el.stop();
    t.join();

    return 0;
}
