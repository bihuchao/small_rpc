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

TEST(Protocol, test_protocol) {
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

TEST(Protocol, test_http) {
    TestReadBuffer rd_buf;
    small_rpc::HTTPProtocol http;

    rd_buf.set_buf("GET /hello/world HTTP/1.1\r\n"
        "Content-Length: 10\r\n"
        "Connection: Keep-alive\r\n"
        "\r\n"
        "helloworld");

    EXPECT_EQ(http.judge_protocol(rd_buf), true);

    small_rpc::Context* ctx = http.new_context();
    small_rpc::HTTPContext* http_ctx = dynamic_cast<small_rpc::HTTPContext*>(ctx);
    EXPECT_TRUE(http_ctx != nullptr);
    LOG_DEBUG << small_rpc::ParseResult_Name(ctx->parse(rd_buf));
    LOG_DEBUG << *ctx;
    EXPECT_EQ(http_ctx->conn_type(), small_rpc::ConnType_Single);
    EXPECT_EQ(http_ctx->service().str(), "hello");
    EXPECT_EQ(http_ctx->method().str(), "world");
}