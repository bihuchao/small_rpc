// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include <assert.h>
#include <gflags/gflags.h>
#include "base/logging.h"
#include "protocols/simple.h"
#include "protocols/http.h"
#include "net/eventloop.h"
#include "net/pb_client.h"
#include "net/pb_controller.h"
#include "echo.pb.h"
#include <thread>
#include <google/protobuf/service.h>

// 选择 simple / http 协议
DEFINE_bool(use_simple_protocol_rather_than_http_protocol, true,
    "use simple protocol rather than http protocol");

void recv_resp(example::EchoRequest* req, example::EchoResponse* resp) {
    LOG_NOTICE << "in recv_resp";
    LOG_NOTICE << "request: " << req->DebugString();
    LOG_NOTICE << "response: " << resp->DebugString();
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, false);
    google::SetCommandLineOption("flagfile", "conf/app.flags");
    std::shared_ptr<void> gflags_guard(nullptr, [](void*){
        LOG_DEBUG << "defer gflags";
        google::ShutDownCommandLineFlags();
    });

    small_rpc::EventLoop el;
    std::thread t(&small_rpc::EventLoop::loop, &el);
    small_rpc::PbClient client(&el, "127.0.0.1", 8878);
    while (!client.connected()) {
        LOG_NOTICE << " wait to connected";
        usleep(20 * 1000);
    }

    if (FLAGS_use_simple_protocol_rather_than_http_protocol) {
        LOG_NOTICE << "client use simple protocol";
        assert(client.set_protocol(new small_rpc::SimpleProtocol()));
    } else {
        LOG_NOTICE << "client use http protocol";
        assert(client.set_protocol(new small_rpc::HTTPProtocol()));
    }

    small_rpc::PbController cntl;
    example::EchoService_Stub stub(&client);
    example::EchoRequest req;
    req.set_logid(1000);
    req.set_message("helloworld");
    LOG_DEBUG << "req: " << req.DebugString();
    example::EchoResponse resp;

    // 异步调用
    ::google::protobuf::Closure* done = ::google::protobuf::NewCallback(recv_resp, &req, &resp);
    stub.echo(&cntl, &req, &resp, done);

    sleep(10);
    LOG_DEBUG << "resp: " << resp.DebugString();
    LOG_DEBUG << "End";
    el.stop();
    t.join();
    LOG_NOTICE << "end main";
    return 0;
}
