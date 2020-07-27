// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "timestamp.h"
#include <iostream>
#include "logging.h"
#include <gflags/gflags.h>
#include <gtest/gtest.h>

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

TEST(TimeStamp, test_len) {
    std::string ts_str = small_rpc::TimeStamp::now().str();
    LOG_DEBUG << "ts_str: " << ts_str;
    EXPECT_EQ(15, ts_str.length());
}

TEST(TimeStamp, test_ostream) {
    std::ostringstream os;
    os << small_rpc::TimeStamp::now();
    std::string os_str = os.str();
    LOG_DEBUG << "os_str: " << os_str;
    EXPECT_EQ(23, os_str.length());
}