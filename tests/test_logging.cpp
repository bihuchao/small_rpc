// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "logging.h"
#include <stdlib.h> // rand

#include <thread>
#include <vector>

void func(int thread_no) {
    for (int i = 0; i < 100; ++i) {
        LOG_NOTICE << "in sub thread" << thread_no;
    }
}

int main(int argc, char** argv) {
    if (!small_rpc::init_log("log/app.log", small_rpc::DEBUG, 1)) {
        fprintf(stderr, "failed to init_log\n");
        return 1;
    }

    LOG_DEBUG << "debug message";
    LOG_NOTICE << "notice message";
    LOG_WARNING << "warning message";
    LOG_FATAL << "fatal message";

    std::vector<std::thread> ts;
    for (int i = 0; i < 10; ++i) {
        ts.emplace_back(func, i);
    }
    for (auto& t : ts) {
        t.join();
    }

    return 0;
}
