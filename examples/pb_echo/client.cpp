// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

// TODO
#include "base/logging.h"
#include "protocols/simple.h"
#include "net/pb_client.h"
#include "net/pb_controller.h"
#include "echo.pb.h"

int main(int argc, char** argv) {
    small_rpc::PbClient client("127.0.0.1", 8878);
    // TODO 指定client协议
    // small_rpc::SimpleProtocol* sm  = new small_rpc::SimpleProtocol;
    small_rpc::PbController cntl;
    example::EchoService_Stub stub(&client);
    example::EchoRequest req;
    req.set_logid(1000);
    req.set_message("helloworld");
    LOG_DEBUG << "req: " << req.DebugString();
    example::EchoResponse resp;
    // 同步调用
    stub.echo(&cntl, &req, &resp, nullptr);
    LOG_DEBUG << "resp: " << resp.DebugString();

    return 0;
}
