// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "server.h"
#include "echo.pb.h"
#include "protocols/simple.h"
#include "logging.h"

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
    small_rpc::Server server("0.0.0.0", 8878);
    assert(server.add_protocol(new small_rpc::SimpleProtocol()));
    assert(server.add_service(new example::EchoServiceImpl()));
    server.start();
    server.el().loop();

    return 0;
}
