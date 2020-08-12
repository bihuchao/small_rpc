// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "pb_client.h"
#include "base/logging.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "socket.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <buffer.h>
#include <protocols/simple.h>

namespace small_rpc {

// PbClient
// TODO 支持长连接
// TODO 支持多协议
// PbClient
PbClient::PbClient(const char* addr, unsigned short port) : _fd(-1) {
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd == -1) {
        PLOG_FATAL << "failed to invoke socket";
    }
    struct sockaddr_in client_addr = get_addr(addr, port);
    int err = connect(_fd, reinterpret_cast<struct sockaddr*>(&client_addr), sizeof(client_addr));
    if (err == -1) {
        PLOG_FATAL << "failed to invoke connect";
    }
}

// handle_events
void PbClient::handle_events(int events) {
    ;
}

// CallMethod
void PbClient::CallMethod(const ::google::protobuf::MethodDescriptor* method,
        ::google::protobuf::RpcController* cntl, const ::google::protobuf::Message* request,
        ::google::protobuf::Message* response, ::google::protobuf::Closure* done) {

    SimpleProtocol sp;
    Context* ctx = sp.new_context();
    ctx->set_conn_type(ConnType_Short);
    ctx->set_method(method->name());
    ctx->set_service(method->service()->name());

    request->SerializeToString(ctx->mutable_payload());
    if (ctx->payload().length() > UINT32_MAX) {
        LOG_WARNING << "req_str length beyond UINT32_MAX";
        return;
    }

    Buffer wr_buf;
    sp.pack_request(wr_buf, ctx);

    while (wr_buf.readable() || wr_buf.extraable()) {
        wr_buf.write_fd(_fd);
    }

    Buffer rd_buf;
    while (true) {
        rd_buf.read_fd(_fd);
        ParseProtocolStatus ret = sp.parse_response(rd_buf, ctx);
        if (ret == ParseProtocol_Error) {
            LOG_WARNING << "ParseProtocol_Error";
            return ;
        }
        if (ret == ParseProtocol_Success) {
            break ;
        }
    }
    std::string str = ctx->payload_view().str();
    response->ParseFromString(str);

    return ;
}

}; // namespace small_rpc
