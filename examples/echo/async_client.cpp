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
#include <condition_variable>

// 选择 simple / http 协议
DEFINE_bool(use_simple_protocol_rather_than_http_protocol, true,
    "use simple protocol rather than http protocol");

small_rpc::EventLoop el;
bool get_resp = false;
std::mutex mtx;
std::condition_variable cv;

void recv_resp(small_rpc::PbController* cntl, example::EchoResponse* resp) {
    LOG_NOTICE << "in recv_resp";
    LOG_NOTICE << "response: " << resp->DebugString();

    std::unique_ptr<small_rpc::PbController> cntll(cntl);
    std::unique_ptr<example::EchoResponse> respp(resp);
    {
        std::unique_lock<std::mutex> lg(mtx);
        get_resp = true;
        cv.notify_all();
    }
}

void async_call(small_rpc::PbClient& client) {
    small_rpc::PbController* cntl = new small_rpc::PbController();
    example::EchoService_Stub stub(&client);
    example::EchoRequest req;
    req.set_logid(1000);
    req.set_message("helloworld");
    LOG_DEBUG << "req: " << req.DebugString();
    example::EchoResponse* resp = new example::EchoResponse();

    // 异步调用
    ::google::protobuf::Closure* done = ::google::protobuf::NewCallback(recv_resp, cntl, resp);
    stub.echo(cntl, &req, resp, done);
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, false);
    google::SetCommandLineOption("flagfile", "conf/app.flags");
    std::shared_ptr<void> gflags_guard(nullptr, [](void*){
        LOG_DEBUG << "defer gflags";
        google::ShutDownCommandLineFlags();
    });

    // eventloop thread
    std::thread t(&small_rpc::EventLoop::loop, &el);
    // pbclient
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

    // async_call
    async_call(client);
    // wait result
    {
        std::unique_lock<std::mutex> lg(mtx);
        while (!get_resp) {
            cv.wait_until(lg, std::chrono::system_clock::now() + std::chrono::milliseconds(500));
        }
    }
    // end
    el.stop();
    t.join();
    LOG_NOTICE << "end main";
    return 0;
}
