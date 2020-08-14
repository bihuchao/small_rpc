// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "buffer.h"
#include "tcpconnection.h"
#include <arpa/inet.h>
#include <google/protobuf/service.h>
#include <atomic>

namespace small_rpc {

class Context;
class Protocol;

// PbClient
class PbClient : public TCPConnection, public ::google::protobuf::RpcChannel {
public:
    // PbClient
    PbClient(const char* addr, unsigned short port);
    PbClient(EventLoop* el, const char* addr, unsigned short port);

    // ~PbClient
    virtual ~PbClient() {
        set_protocol(nullptr);
    }

    // client_conn_callback
    void client_conn_callback();
    // data_read_callback
    void data_read_callback();
    // write_callback
    void write_complete_callback();
    // close_callback
    void close_callback();

    // response_callback
    void response_callback();

    // set_protocol
    bool set_protocol(Protocol* protocol);

    // CallMethod
    void CallMethod(const ::google::protobuf::MethodDescriptor* method,
            ::google::protobuf::RpcController* cntl,
            const ::google::protobuf::Message* request, ::google::protobuf::Message* response, ::google::protobuf::Closure* done);

private:
    Protocol* _protocol;
    struct sockaddr_in _server_addr;
    ::google::protobuf::Message* _response;
    ::google::protobuf::Closure* _done;
    bool _donee;
};

}; // namespace small_rpc
