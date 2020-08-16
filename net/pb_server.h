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
    PbServer(const char* addr, unsigned short port)
            : TCPServer(addr, port), _read_timeout_ms(-1), _write_timeout_ms(-1) {
        _acceptor.set_new_connection_callback(
            std::bind(&PbServer::new_connection_callback, this, std::placeholders::_1));
    }

    ~PbServer();

    // 只能在start之前使用
    bool add_protocol(Protocol* proto);

    bool add_service(::google::protobuf::Service* service);

    void set_read_timeout_ms(int read_timeout_ms) { _read_timeout_ms = read_timeout_ms; }

    void set_write_timeout_ms(int write_timeout_ms) { _write_timeout_ms = write_timeout_ms; }

    void new_connection_callback(int conn);

    void data_read_callback(TCPConnection* conn);

    void write_complete_callback(TCPConnection* conn);

    static void timeout_callback(EventLoop* el, TCPConnection* conn,
        TimeStamp& conn_ts, int timer_id, bool is_read);

    static void close_callback(TCPConnection* conn);

    void request_callback(TCPConnection* conn);

    void response_callback(ReqRespConnPack* pack);

private:
    // find service and method
    bool _find_service_method(const std::string& service_name,
            const std::string& method_name, ::google::protobuf::Service*& service,
            const ::google::protobuf::MethodDescriptor*& method);

private:
    int _read_timeout_ms, _write_timeout_ms;
    std::vector<Protocol*> _protocols;
    std::map<std::string, Protocol*> _name2protocols;
    Services _services;
};

}; // namespace small_rpc
