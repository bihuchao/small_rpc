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

DEFINE_int32(read_timeout_ms, 2000, "read timeout ms");
DEFINE_int32(write_timeout_ms, 2000, "write timeout ms");
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

    LOG_DEBUG << "in main";
    small_rpc::PbServer pb_server("0.0.0.0", 8878);
    // 支持单端口多协议
    assert(pb_server.add_protocol(new small_rpc::SimpleProtocol()));
    assert(pb_server.add_protocol(new small_rpc::HTTPProtocol()));
    assert(pb_server.add_service(new example::EchoServiceImpl()));
    pb_server.set_read_timeout_ms(FLAGS_read_timeout_ms);
    pb_server.set_write_timeout_ms(FLAGS_write_timeout_ms);
    pb_server.set_thread_num(FLAGS_server_thread_num);
    pb_server.start();

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
