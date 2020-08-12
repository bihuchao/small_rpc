// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include <iostream>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "base/logging.h"
#include "base/timestamp.h"
#include "buffer.h"

DEFINE_uint64(empty_size, 97, "empty size");

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


TEST(Buffer, test_read_buffer) {
    // ctor
    small_rpc::Buffer buf(12);
    buf.append("helloworld");
    EXPECT_EQ(buf.readable(), 10);
    EXPECT_EQ(buf.writeable(), 2);

    // expand
    // 12 => 24
    buf.append(0x19951028);
    EXPECT_EQ(buf.readable(), 14);
    EXPECT_EQ(buf.writeable(), 10);

    // bufview
    small_rpc::BufferView bufview(buf, 5);
    EXPECT_EQ(bufview.str(), "hello");
    small_rpc::BufferView bufview2(buf, 10);
    EXPECT_EQ(bufview2.str(), "helloworld");

    // retrieve
    buf.retrieve(10);
    EXPECT_EQ(buf.readable(), 4);
    EXPECT_EQ(buf.writeable(), 10);

    // peek_int32
    int32_t num = buf.peek_int32();
    EXPECT_EQ(0x19951028, num);
    buf.retrieve(sizeof(int32_t));
    EXPECT_EQ(buf.readable(), 0);
    EXPECT_EQ(buf.writeable(), 10);

    // expend
    buf.append("world");
    EXPECT_EQ(buf.readable(), 5);
    EXPECT_EQ(buf.writeable(), 5);
    buf.append("hello");
    EXPECT_EQ(buf.readable(), 10);
    EXPECT_EQ(buf.writeable(), 0);
    // 24 => 48
    buf.append("1");
    EXPECT_EQ(buf.readable(), 11);
    EXPECT_EQ(buf.writeable(), 23);
}

TEST(Buffer, test_find) {
    size_t empty = FLAGS_empty_size;
    small_rpc::Buffer rd_buf(12 + empty);
    rd_buf.append("hello\r\nworld");
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
    size_t empty = FLAGS_empty_size;
    small_rpc::Buffer rd_buf(empty + 12);
    rd_buf.append("hello\r\nworld");
    small_rpc::BufferView buf_view(&rd_buf,0, 5);
    EXPECT_EQ("hello", buf_view.str());
    small_rpc::BufferView buf_view2(&rd_buf,7, 12);
    EXPECT_EQ("world", buf_view2.str());
}
