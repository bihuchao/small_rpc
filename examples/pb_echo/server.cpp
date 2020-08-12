// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "protocols/simple.h"
#include "base/logging.h"
#include "base/status_manager.h"
#include "net/pb_server.h"
#include "echo.pb.h"

namespace example {

class EchoServiceImpl : public example::EchoService {
public:
    void echo(::google::protobuf::RpcController* controller,
            const ::example::EchoRequest* request,
            ::example::EchoResponse* response,
            ::google::protobuf::Closure* done) {
        LOG_NOTICE << "enter EchoServiceImpl echo";
        LOG_DEBUG << "request: " << request->DebugString();
        response->set_logid(request->logid());
        response->set_result(request->message() + " powered by EchoService");
        LOG_NOTICE << "exit EchoServiceImpl echo";
        done->Run();
    }
};

}; // namespace example

int main(int argc, char** argv) {
    LOG_DEBUG << "in main";
    small_rpc::PbServer pb_server("0.0.0.0", 8878);
    assert(pb_server.add_protocol(new small_rpc::SimpleProtocol()));
    assert(pb_server.add_service(new example::EchoServiceImpl()));
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
