// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "logging.h"
#include <stdlib.h> // rand

#include <thread>
#include <vector>
#include <gflags/gflags.h>
#include <gtest/gtest.h>

DEFINE_int32(thread_num, 2, "thread num");
DEFINE_int32(log_count, 5, "log count num");

void func(int thread_no) {
    for (int i = 0; i < FLAGS_log_count; ++i) {
        LOG_NOTICE << "in sub thread" << thread_no;
    }
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, false);
    google::SetCommandLineOption("flagfile", "conf/app.flags");
    std::shared_ptr<void> gflags_guard(nullptr, [](void*){
        LOG_DEBUG << "defer gflags";
        google::ShutDownCommandLineFlags();
    });

    if (!small_rpc::init_log("log/app.log", small_rpc::DEBUG)) {
        fprintf(stderr, "failed to init_log\n");
        return 1;
    }

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(logging, test_severity) {
    // LOG_*
    LOG_DEBUG << "debug message";
    LOG_NOTICE << "notice message";
    LOG_WARNING << "warning message";
    // LOG_FATAL << "fatal message";

    // PLOG_*
    PLOG_NOTICE << "plog notice message";
    // PLOG_FATAL << "fatal message";

    // PLOG_*_IF
    PLOG_DEBUG_IF(true) << "plog true debug message";
    PLOG_DEBUG_IF(false) << "plog false debug message";
    PLOG_NOTICE_IF(true) << "plog true notice message";
    PLOG_NOTICE_IF(false) << "plog false notice message";
}

TEST(logging, test_multithread) {
    std::vector<std::thread> ts;
    for (int i = 0; i < FLAGS_thread_num; ++i) {
        ts.emplace_back(func, i);
    }
    for (auto& t : ts) {
        t.join();
    }
}
