// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include <iostream>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "base/timestamp.h"
#include "base/logging.h"
#include "protocols/http.h"

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

TEST(HTTPProtocol, test_parse_request) {
    small_rpc::HTTPProtocol http;
    {
        small_rpc::Context* ctx = nullptr;
        small_rpc::Buffer rd_buf;
        rd_buf.append("GET HTTP/1.1 /hello/world");
        EXPECT_EQ(http.parse_request(rd_buf, &ctx), small_rpc::ParseProtocol_NoEnoughData);
        EXPECT_TRUE(ctx != nullptr);
        LOG_DEBUG << "debug context: " << *ctx;
        delete ctx;
    }
    {
        small_rpc::Context* ctx = nullptr;
        small_rpc::Buffer rd_buf;
        rd_buf.append("OPTIONS HTTP/1.1 /hello/world");
        EXPECT_EQ(http.parse_request(rd_buf, &ctx), small_rpc::ParseProtocol_NoEnoughData);
        EXPECT_TRUE(ctx != nullptr);
        LOG_DEBUG << "debug context: " << *ctx;
        delete ctx;
    }
    {
        small_rpc::Context* ctx = nullptr;
        small_rpc::Buffer rd_buf;
        rd_buf.append("FAKE HTTP/1.1 /hello/world");
        EXPECT_EQ(http.parse_request(rd_buf, &ctx), small_rpc::ParseProtocol_TryAnotherProtocol);
        EXPECT_TRUE(ctx == nullptr);
    }
}

TEST(HTTPContext, test_parse_request) {
    small_rpc::HTTPProtocol http;
    small_rpc::Buffer rd_buf;
    rd_buf.append("GET /hello/world HTTP/1.1\r\n"
        "Content-Length: 10\r\n"
        "Connection: Keep-alive\r\n"
        "\r\n"
        "helloworld");
    small_rpc::Context* ctx = nullptr;
    EXPECT_EQ(http.parse_request(rd_buf, &ctx), small_rpc::ParseProtocol_Success);
    EXPECT_TRUE(ctx != nullptr);
    LOG_DEBUG << "debug context: " << *ctx;

    small_rpc::HTTPContext* http_ctx = dynamic_cast<small_rpc::HTTPContext*>(ctx);
    EXPECT_TRUE(http_ctx != nullptr);
    EXPECT_EQ(http_ctx->conn_type(), small_rpc::ConnType_Single);
    EXPECT_EQ(http_ctx->service(), "hello");
    EXPECT_EQ(http_ctx->method(), "world");
    delete ctx;
}

TEST(HTTPContext, test_pack_request) {
    small_rpc::HTTPProtocol http;
    small_rpc::Buffer wr_buf;

    small_rpc::Context* ctx = http.new_context();
    EXPECT_TRUE(ctx != nullptr);
    ctx->set_conn_type(small_rpc::ConnType_Single);
    ctx->set_service("hello");
    ctx->set_method("world");
    ctx->set_payload("helloworld");
    ctx->set_rpc_status(small_rpc::RpcStatus_OK);
    ctx->pack_request(wr_buf);
    delete ctx;
}

TEST(HTTPContext, test_parse_response) {
    small_rpc::HTTPProtocol http;
    small_rpc::Buffer rd_buf;
    rd_buf.append("HTTP/1.1 200 OK\r\n"
        "Content-Length: 10\r\n"
        "Connection: Keep-alive\r\n"
        "\r\n"
        "helloworld");
    small_rpc::Context* ctx = http.new_context();
    EXPECT_TRUE(ctx != nullptr);
    EXPECT_EQ(ctx->parse_response(rd_buf), small_rpc::ParseProtocol_Success);
    LOG_DEBUG << "debug context: " << *ctx;

    small_rpc::HTTPContext* http_ctx = dynamic_cast<small_rpc::HTTPContext*>(ctx);
    EXPECT_TRUE(http_ctx != nullptr);
    delete ctx;
}
