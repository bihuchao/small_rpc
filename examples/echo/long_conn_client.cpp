// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include <assert.h>
#include <gflags/gflags.h>
#include "base/logging.h"
#include "protocols/simple.h"
#include "protocols/http.h"
#include "net/pb_client.h"
#include "net/pb_controller.h"
#include "echo.pb.h"

// 选择 simple / http 协议
DEFINE_bool(use_simple_protocol_rather_than_http_protocol, true,
    "use simple protocol rather than http protocol");

// 长连接CallMethod次数
DEFINE_int32(call_method_num, 10, "call method num");

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, false);
    google::SetCommandLineOption("flagfile", "conf/app.flags");
    std::shared_ptr<void> gflags_guard(nullptr, [](void*){
        LOG_DEBUG << "defer gflags";
        google::ShutDownCommandLineFlags();
    });
    signal(SIGPIPE, SIG_IGN);

    small_rpc::PbClient client("127.0.0.1", 8878);
    if (!client.connected()) {
        LOG_WARNING << "client is not connected.";
        return 0;
    }
    if (FLAGS_use_simple_protocol_rather_than_http_protocol) {
        LOG_NOTICE << "client use simple protocol";
        assert(client.set_protocol(new small_rpc::SimpleProtocol()));
    } else {
        LOG_NOTICE << "client use http protocol";
        assert(client.set_protocol(new small_rpc::HTTPProtocol()));
    }
    small_rpc::PbController cntl;
    // 长连接
    cntl.set_conn_type(small_rpc::ConnType_Single);
    example::EchoService_Stub stub(&client);

    for (int i = 0; i < FLAGS_call_method_num; ++i) {
        example::EchoRequest req;
        req.set_logid(1000 + i);
        req.set_message("helloworld " + std::to_string(i));
        LOG_DEBUG << "req: " << req.DebugString();
        example::EchoResponse resp;
        // 同步调用
        stub.echo(&cntl, &req, &resp, nullptr);
        LOG_DEBUG << "resp: " << resp.DebugString();
    }
    return 0;
}
