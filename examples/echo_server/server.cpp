#include "server.h"
#include "echo.pb.h"
#include "protocol/http.h"
#include "logging.h"

namespace example {

class EchoServiceImpl : public example::EchoService {
public:
    void echo(::google::protobuf::RpcController* controller,
            const ::example::EchoRequest* request,
            ::example::EchoResponse* response,
            ::google::protobuf::Closure* done) {
        LOG_NOTICE << "enter EchoServiceImpl echo";
        response->set_logid(request->logid());
        response->set_result("EchoService" + request->message());
        LOG_NOTICE << "exit EchoServiceImpl echo";
        done->Run();
    }
};

}; // namespace example

int main(int argc, char** argv) {
    small_rpc::Server server("0.0.0.0", 8878);
    assert(server.add_protocol(new small_rpc::HTTPProtocol()));
    assert(server.add_service(new example::EchoServiceImpl()));
    server.start();
    server.el().loop();

    return 0;
}
