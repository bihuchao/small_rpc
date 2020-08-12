// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "server.h"
#include "logging.h"

namespace small_rpc {

// ~Server
Server::~Server() {
    for (auto& protocol : _protocols) {
        delete protocol;
    }
    for (auto& p : _services) {
        delete p.second.first;
    }
}

// add_protocol
bool Server::add_protocol(Protocol* proto) {
    std::map<std::string, Protocol*>::iterator iter = _name2protocols.find(proto->name());
    if (iter != _name2protocols.end()) {
        return false;
    }
    _name2protocols[proto->name()] = proto;
    _protocols.push_back(proto);
    return true;
}

// add_service
bool Server::add_service(::google::protobuf::Service* service) {
    const ::google::protobuf::ServiceDescriptor* sd = service->GetDescriptor();
    auto it = _services.find(sd->name());
    if (it != _services.end()) {
        return false;
    }
    _services[sd->name()] = std::make_pair(service, ServiceMethods());
    auto& methods = _services[sd->name()].second;
    for (int i = 0; i < sd->method_count(); ++i) {
        const ::google::protobuf::MethodDescriptor* md = sd->method(i);
        auto it = methods.find(md->name());
        if (it != methods.end()) {
            return false;
        }
        methods[md->name()] = md;
    }

    return true;
}

// 另起线程开始run
bool Server::start() {
    if (_thread_num > 0) {
        // 启动 sub_reactor 线程池，线程数为 _thread_num
        // TODO
        LOG_WARNING << "not support multi-thread mode now.";
        return false;
    }
    // 启动 main_reactor 线程
    _main_reactor_thread = std::thread(&EventLoop::loop, &_el);
    return true;
}

bool Server::stop() {
    // TODO
    // 停止 sub_reactor线程池
    _el.stop();
    if (_main_reactor_thread.joinable()) {
        _main_reactor_thread.join();
    }
    return true;
}

// find service and method
bool Server::_find_service_method(const std::string& service_name,
        const std::string& method_name, ::google::protobuf::Service*& service,
        const ::google::protobuf::MethodDescriptor*& method) {

    // find service
    auto it = _services.find(service_name);
    if (it == _services.end()) {
        LOG_WARNING << "Failed to find service: " << service_name;
        return false;
    }
    service = it->second.first;
    // find method
    ServiceMethods& methods = it->second.second;
    auto it2 = methods.find(method_name);
    if (it2 == methods.end()) {
        LOG_WARNING << "Failed to find method: " << method_name
            << ", service: " << service_name;
        return false;
    }
    method = it2->second;

    return true;
}

// new_connection_callback
void Server::new_connection_callback(TCPConnection* conn) {
    LOG_DEBUG << "in server new_connection_callback";
    conn->set_data_read_callback(
        std::bind(&Server::data_read_callback, this, std::placeholders::_1));
    conn->set_close_callback(
        std::bind(&Server::close_callback, this, std::placeholders::_1));
    conn->set_write_complete_callback(
        std::bind(&Server::write_complete_callback, this, std::placeholders::_1));
}

// data_read_callback
void Server::data_read_callback(TCPConnection* conn) {
    LOG_DEBUG << "in server data_read_callback";

    ParseProtocolStatus res;
    while (conn->_protocol < _protocols.size()) {
        res = _protocols[conn->_protocol]->parse_request(conn->_rbuf, conn->mutable_context());
        if (res == ParseProtocol_TryAnotherProtocol) {
            ++conn->_protocol;
        } else {
            break;
        }
    }
    if (conn->_protocol >= _protocols.size()) {
        LOG_WARNING << "can't parse with all protocol";
        conn->close();
        return ;
    }
    if (res == ParseProtocol_Error) {
        LOG_WARNING << "can't parse with protocol " << _protocols[conn->_protocol]->name();
        conn->close();
        return ;
    }
    if (res == ParseProtocol_NoEnoughData) {
        return ;
    }
    LOG_DEBUG << "get parse context: " << *conn->context();

    // 这里分包成功了, 开始调用 request_callback
    conn->set_status(small_rpc::TCPConnection_ReadSuccess);
    request_callback(conn);
}

// write_complete_callback
void Server::write_complete_callback(TCPConnection* conn) {
    // TODO 判断 短连接 / 长连接
    conn->set_event(0);
    conn->el()->update_channel(static_cast<Channel*>(conn));
    close(conn->fd());
    LOG_DEBUG <<"finish to write";
}

// close_callback
void Server::close_callback(TCPConnection* conn) {
    // TODO 判断 短连接 / 长连接
    conn->set_event(0);
    conn->el()->update_channel(static_cast<Channel*>(conn));
    close(conn->fd());
    LOG_DEBUG <<"in close_callback";
}

// request_callback
// message分包完毕
void Server::request_callback(TCPConnection* conn) {
    // 执行 应用层函数
    const std::string& pb_data = conn->context()->payload_view().str();
    const std::string& service_name = conn->context()->service();
    const std::string& method_name = conn->context()->method();
    LOG_DEBUG << "in request_callback: " << service_name << " " << method_name;

    ::google::protobuf::Service* service = nullptr;
    const ::google::protobuf::MethodDescriptor* method = nullptr;
    if (!_find_service_method(service_name, method_name, service, method)) {
        // TODO 发送错误代码 关闭客户端
        return ;
    }

    // new message and parse request and prepare done
    google::protobuf::Message* request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(pb_data)) {
        delete request;
        // TODO 解析失败 关闭客户端
        return ;
    }
    google::protobuf::Message* response = service->GetResponsePrototype(method).New();

    ReqRespConnPack* pack = new ReqRespConnPack(request, response, conn);

    ::google::protobuf::Closure* done = ::google::protobuf::NewCallback(this,
        &small_rpc::Server::response_callback, pack);

    // call method
    service->CallMethod(method, nullptr, request, response, done);
}

// response_callback
void Server::response_callback(ReqRespConnPack* pack) {
    LOG_DEBUG << "in response_callback";

    // TODO 智能指针
    std::string resp_str;
    assert(pack->_response->SerializeToString(&resp_str));
    delete pack->_request;
    delete pack->_response;
    TCPConnection* conn = pack->_conn;
    delete pack;

    conn->_ctx->set_rpc_status(RpcStatus_OK);
    *conn->_ctx->mutable_payload() = std::move(resp_str);
    bool ret = _protocols[conn->_protocol]->pack_response(conn->_wbuf, conn->context());
    // TODO judge ret

    conn->set_event(EPOLLOUT);
    // TODO 目前只支持同步调用 异步调用待支持
    conn->el()->update_channel(static_cast<Channel*>(conn));
}

}; // namespace small_rpc
