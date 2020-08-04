// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "timestamp.h"
#include <iostream>
#include "logging.h"
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "connection.pb.h"
#include "protocol/http.h"

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

class TestReadBuffer : public small_rpc::ReadBuffer {
public:
    void set_buf(const char* data) {
        int len = strlen(data);
        _data.resize(len);
        strncpy(&_data[0], data, len);
        _windex = len;
    }
};

TEST(Protocol, test_http) {
    TestReadBuffer rd_buf;
    small_rpc::HTTPProtocol http;

    rd_buf.set_buf("GET HTTP/1.1 /hello/world");
    EXPECT_EQ(http.judge_protocol(rd_buf), true);

    rd_buf.set_buf("POST HTTP/1.1 /hello/world");
    EXPECT_EQ(http.judge_protocol(rd_buf), true);

    rd_buf.set_buf("OPTIONS HTTP/1.1 /hello/world");
    EXPECT_EQ(http.judge_protocol(rd_buf), true);

    rd_buf.set_buf("fake method");
    EXPECT_EQ(http.judge_protocol(rd_buf), false);
}
