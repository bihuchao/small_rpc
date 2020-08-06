// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "timestamp.h"
#include <iostream>
#include "logging.h"
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "buffer.h"

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
    void set_buf(const char* data, size_t empty = 10) {
        int len = strlen(data);
        _data.resize(len + empty);
        strncpy(&_data[0], data, len);
        _windex = len;
    }
};

TEST(Buffer, test_read_buffer) {
    TestReadBuffer rd_buf;
    size_t empty = 100;
    rd_buf.set_buf("hello\r\nworld", empty);
    EXPECT_EQ(rd_buf.readable(), 12);
    EXPECT_EQ(rd_buf.writeable(), empty);
    EXPECT_EQ(rd_buf.find_crlf(12), 5);

    rd_buf.retrieve(2);
    EXPECT_EQ(rd_buf.readable(), 10);
    EXPECT_EQ(rd_buf.writeable(), empty);
    EXPECT_EQ(rd_buf.find_crlf(10), 3);

    rd_buf.retrieve(3);
    EXPECT_EQ(rd_buf.readable(), 7);
    EXPECT_EQ(rd_buf.writeable(), empty);
    EXPECT_EQ(rd_buf.find_crlf(7), 0);

    rd_buf.retrieve(1);
    EXPECT_EQ(rd_buf.readable(), 6);
    EXPECT_EQ(rd_buf.writeable(), empty);
    EXPECT_EQ(rd_buf.find_crlf(6), -1);
}

TEST(Buffer, test_buffer_view) {
    TestReadBuffer rd_buf;
    size_t empty = 100;
    rd_buf.set_buf("hello\r\nworld", empty);
    small_rpc::BufferView buf_view(&rd_buf,0, 5);
    LOG_DEBUG << buf_view.str();
    small_rpc::BufferView buf_view2(&rd_buf,7, 12);
    LOG_DEBUG << buf_view2.str();
}
