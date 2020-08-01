// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "timestamp.h"
#include <iostream>
#include "logging.h"
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "errno.pb.h"

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

TEST(ClientInfo, test_print) {
    small_rpc::ClientInfo client_info;
    client_info.set_id(12345678);
    client_info.set_client("http client");
    LOG_DEBUG << client_info.DebugString();
}
