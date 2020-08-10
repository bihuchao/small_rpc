// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>
#include "protocol.h"
#include "eventloop.h"
#include "tcpconnection.h"
#include "acceptor.h"
#include <signal.h>

namespace small_rpc {

// Server
// TODO 拆分 TCPServer 和 PBRPCServer
class Server {
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
    friend class Server;
    };

public:
    using ServiceMethods = std::map<std::string, const ::google::protobuf::MethodDescriptor*>;
    using Service = std::pair<::google::protobuf::Service*, ServiceMethods>;
    using Services = std::map<std::string, Service>;

public:
    Server(const char* addr, unsigned short port) : _acceptor(&_el, addr, port), _thread_num(0) {
        _acceptor.set_new_connection_callback(
            std::bind(&Server::new_connection_callback, this, std::placeholders::_1));
    }

    EventLoop& el() { return _el; }

    void set_thread_num(size_t thread_num) { _thread_num = thread_num; }

    bool start() {
        // 在本线程中启动server
        if (_thread_num == 0) {
            // if (!_register_sigint()) { return false; }
            _el.loop();
        }
        return true;
    }

    bool add_protocol(Protocol* proto);

    bool add_service(::google::protobuf::Service* service);

    void new_connection_callback(TCPConnection* conn);

    void data_read_callback(TCPConnection* conn);

    void request_callback(TCPConnection* conn);

    void response_callback(ReqRespConnPack* pack);

    void write_complete_callback(TCPConnection* conn);

private:
    // find service and method
    bool _find_service_method(const std::string& service_name,
            const std::string& method_name, ::google::protobuf::Service*& service,
            const ::google::protobuf::MethodDescriptor*& method);

    // TODO register_sigint

private:
    std::vector<Protocol*> _protocols;
    std::map<std::string, Protocol*> _name2protocols;
    Services _services;
    EventLoop _el;
    Acceptor _acceptor;
    size_t _thread_num;
};

}; // namespace small_rpc
