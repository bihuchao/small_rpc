// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "timestamp.h"
#include <iostream>
#include "logging.h"
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include "socket.h"

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

namespace small_rpc {

TEST(ClientInfo, test_print) {
    auto addr = get_addr("127.0.0.0", 8878);
    LOG_NOTICE << "first addr: " << addr;

    auto addr2 = get_addr("192.168.1.1", 1028);
    LOG_NOTICE << "second addr: " << addr2;
}

}; // namespace small_rpc
