// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "timestamp.h"
#include <iostream>

int main(int argc, char** argv) {
    std::cout << small_rpc::TimeStamp::now() << std::endl;
    std::cout << small_rpc::TimeStamp::now().str() << std::endl;
    return 0;
}
