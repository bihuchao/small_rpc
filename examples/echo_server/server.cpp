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

bool is_stop = false;

void deal_sig_int(int signo) {
    LOG_NOTICE << "in deal_sig_int: " << signo;
    is_stop = true;
}

int main(int argc, char** argv) {
    LOG_DEBUG << "in main";
    small_rpc::Server server("0.0.0.0", 8878);
    assert(server.add_protocol(new small_rpc::SimpleProtocol()));
    assert(server.add_service(new example::EchoServiceImpl()));
    server.start();

    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = deal_sig_int;
    int err = sigaction(SIGINT, &sa, nullptr);
    PLOG_FATAL_IF(err == -1) << "failed to invoke sigaction";

    while (!is_stop) {
        ::sleep(5);
    }

    LOG_DEBUG << "stop server";
    server.stop();
    LOG_DEBUG << "end main";

    return 0;
}
