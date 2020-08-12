// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include <iostream>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "base/logging.h"
#include "base/timestamp.h"
#include "protocol.pb.h"
#include "protocols/simple.h"

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

TEST(SimpleProtocol, test_parse_protocol) {
    small_rpc::Buffer buf;
    small_rpc::SimpleProtocol sp;
    small_rpc::Context* req_ctx = nullptr;

    buf.append("1");
    EXPECT_EQ(sp.parse_request(buf, &req_ctx), small_rpc::ParseProtocol_NoEnoughData);
    buf.retrieve(1);
    EXPECT_TRUE(req_ctx == nullptr);

    buf.append(sp.MAGIC_NUM - 1);
    EXPECT_EQ(sp.parse_request(buf, &req_ctx), small_rpc::ParseProtocol_TryAnotherProtocol);
    buf.retrieve(sizeof(sp.MAGIC_NUM));
    EXPECT_TRUE(req_ctx == nullptr);

    buf.append(sp.MAGIC_NUM);
    EXPECT_EQ(sp.parse_request(buf, &req_ctx), small_rpc::ParseProtocol_NoEnoughData);
    buf.retrieve(sizeof(sp.MAGIC_NUM));
    EXPECT_TRUE(req_ctx != nullptr);
    delete req_ctx;
}

TEST(SimpleProtocol, test_parse_context) {
    small_rpc::Buffer buf;
    small_rpc::SimpleProtocol sp;
    small_rpc::Context* req_ctx = nullptr;

    buf.append(sp.MAGIC_NUM);
    buf.append(small_rpc::ConnType_Short);
    buf.append(16);
    buf.append("EchoService/echo");

    // buf.append(5);
    // buf.append("/echo");

    // buf.append(12);
    // buf.append("EchoService/");

    // buf.append(1);
    // buf.append("/");

    buf.append(10);
    buf.append("helloworld");

    EXPECT_EQ(sp.parse_request(buf, &req_ctx), small_rpc::ParseProtocol_Success);
    LOG_DEBUG << *req_ctx;
}
