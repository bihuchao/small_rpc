// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include <gflags/gflags.h>
#include "base/logging.h"
#include "base/done_guard.h"
#include "base/status_manager.h"
#include "protocols/simple.h"
#include "protocols/http.h"
#include "net/pb_server.h"
#include "echo.pb.h"

DEFINE_int32(server_thread_num, 2, "server thread num");

namespace example {

// EchoServiceImpl
class EchoServiceImpl : public example::EchoService {
public:
    void echo(::google::protobuf::RpcController* controller,
            const ::example::EchoRequest* request,
            ::example::EchoResponse* response,
            ::google::protobuf::Closure* done) {
        small_rpc::DoneGuard done_guard(done);
        LOG_NOTICE << "enter EchoServiceImpl echo";
        usleep(1000 * 1000);
        LOG_DEBUG << "request: " << request->DebugString();
        response->set_logid(request->logid());
        response->set_result(request->message() + " powered by EchoService");
        LOG_NOTICE << "exit EchoServiceImpl echo";
    }
};

}; // namespace example

int main(int argc, char** argv) {
    LOG_DEBUG << "in main";
    small_rpc::PbServer pb_server("0.0.0.0", 8878);
    // 支持单端口多协议
    assert(pb_server.add_protocol(new small_rpc::SimpleProtocol()));
    assert(pb_server.add_protocol(new small_rpc::HTTPProtocol()));
    assert(pb_server.add_service(new example::EchoServiceImpl()));
    pb_server.set_thread_num(FLAGS_server_thread_num);
    pb_server.start();

    LOG_NOTICE << "register signal manager.";
    small_rpc::StatusManager& sm = small_rpc::StatusManager::get_instance();
    sm.register_signals();
    while (!sm.is_close()) {
        sm.wait_for_signal();
    }
    LOG_NOTICE << "stop pb_server";

    pb_server.stop();
    LOG_DEBUG << "end main";

    return 0;
}
