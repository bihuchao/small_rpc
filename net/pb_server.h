// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>
#include "protocols/protocol.h"
#include "tcpconnection.h"
#include "tcpserver.h"

namespace small_rpc {

// PbServer
class PbServer : public TCPServer {
public:
    // google protobuf not support >2 params callback closure
    // so use ReqRespConnPack to pack many params
    class ReqRespConnPack {
        ReqRespConnPack(google::protobuf::Message* request,
            google::protobuf::Message* response, TCPConnection* conn)
                : _request(request), _response(response), _conn(conn) {}
    private:
        google::protobuf::Message* _request;
        google::protobuf::Message* _response;
        TCPConnection* _conn;
    friend class PbServer;
    };

public:
    using ServiceMethods = std::map<std::string, const ::google::protobuf::MethodDescriptor*>;
    using Service = std::pair<::google::protobuf::Service*, ServiceMethods>;
    using Services = std::map<std::string, Service>;

public:
    PbServer(const char* addr, unsigned short port) : TCPServer(addr, port) {
        _acceptor.set_new_connection_callback(
            std::bind(&PbServer::new_connection_callback, this, std::placeholders::_1));
    }

    ~PbServer();

    // 只能在start之前使用
    bool add_protocol(Protocol* proto);

    bool add_service(::google::protobuf::Service* service);

    void new_connection_callback(TCPConnection* conn);

    void data_read_callback(TCPConnection* conn);

    void write_complete_callback(TCPConnection* conn);

    void close_callback(TCPConnection* conn);

    void request_callback(TCPConnection* conn);

    void response_callback(ReqRespConnPack* pack);

private:
    // find service and method
    bool _find_service_method(const std::string& service_name,
            const std::string& method_name, ::google::protobuf::Service*& service,
            const ::google::protobuf::MethodDescriptor*& method);

private:
    std::vector<Protocol*> _protocols;
    std::map<std::string, Protocol*> _name2protocols;
    Services _services;
};

}; // namespace small_rpc
