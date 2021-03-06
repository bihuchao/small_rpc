// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include <iostream>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "base/timestamp.h"
#include "base/logging.h"

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, false);
    google::SetCommandLineOption("flagfile", "conf/app.flags");
    std::shared_ptr<void> gflags_guard(nullptr, [](void*){
        LOG_DEBUG << "defer gflags";
        google::ShutDownCommandLineFlags();
    });

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

TEST(TimeStamp, test_operator_less) {
    small_rpc::TimeStamp ts = small_rpc::TimeStamp::now();
    small_rpc::TimeStamp ts2 = small_rpc::TimeStamp::after_now_ms(900);
    LOG_DEBUG << ts;
    LOG_DEBUG << ts2;
    EXPECT_TRUE(ts < ts2);
}

TEST(TimeStamp, test_operator_reduce) {
    small_rpc::TimeStamp ts = small_rpc::TimeStamp::now();
    small_rpc::TimeStamp ts2 = small_rpc::TimeStamp::after_now_ms(3900);
    EXPECT_EQ(-3900, ts - ts2);
    EXPECT_EQ(3900, ts2 - ts);
}
