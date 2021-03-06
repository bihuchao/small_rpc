// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "pb_server.h"
#include "socket.h"
#include "base/logging.h"

namespace small_rpc {

// ~PbServer
PbServer::~PbServer() {
    for (auto& protocol : _protocols) {
        delete protocol;
    }
    for (auto& p : _services) {
        delete p.second.first;
    }
}

// add_protocol
bool PbServer::add_protocol(Protocol* proto) {
    std::map<std::string, Protocol*>::iterator iter = _name2protocols.find(proto->name());
    if (iter != _name2protocols.end()) {
        return false;
    }
    _name2protocols[proto->name()] = proto;
    _protocols.push_back(proto);
    return true;
}

// add_service
bool PbServer::add_service(::google::protobuf::Service* service) {
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

// find service and method
bool PbServer::_find_service_method(const std::string& service_name,
        const std::string& method_name, ::google::protobuf::Service*& service,
        const ::google::protobuf::MethodDescriptor*& method) {

    // find service
    auto it = _services.find(service_name);
    if (it == _services.end()) {
        LOG_WARNING << "PbServer failed to find service: " << service_name;
        return false;
    }
    service = it->second.first;
    // find method
    ServiceMethods& methods = it->second.second;
    auto it2 = methods.find(method_name);
    if (it2 == methods.end()) {
        LOG_WARNING << "PbServer failed to find method: " << method_name
            << ", service: " << service_name;
        return false;
    }
    method = it2->second;

    return true;
}

// new_connection_callback
void PbServer::new_connection_callback(int conn) {
    TCPConnection* http_conn = new TCPConnection(conn, _get_next_el());
    LOG_NOTICE << "connect from " << http_conn->fd();
    // 添加到channels
    http_conn->el()->add_channel(http_conn, http_conn->ts);
    // 设置非阻塞IO
    set_nonblocking(http_conn->fd());
    // 开启读监听
    http_conn->set_event(EPOLLIN);
    http_conn->el()->update_channel(http_conn);
    if (_read_timeout_ms > 0) {
        // 设置读超时定时器
        http_conn->timer_id.fetch_add(1);
        http_conn->el()->run_after(_read_timeout_ms, std::bind(&PbServer::timeout_callback,
            http_conn->el(), http_conn, http_conn->ts, http_conn->timer_id.load(), true));
    }
    http_conn->set_data_read_callback(
        std::bind(&PbServer::data_read_callback, this, std::placeholders::_1));
    http_conn->set_close_callback(
        std::bind(&PbServer::close_callback, std::placeholders::_1));
    http_conn->set_write_complete_callback(
        std::bind(&PbServer::write_complete_callback, this, std::placeholders::_1));
}

// data_read_callback
void PbServer::data_read_callback(TCPConnection* conn) {
    ParseProtocolStatus res;
    while (conn->proto_idx < _protocols.size()) {
        res = _protocols[conn->proto_idx]->parse_request(conn->rbuf(), conn->mutable_context());
        // 这里面改成 protocol parse_request + context parse_request
        // 这个也不用改，现在这种调用方式还可以
        if (res == ParseProtocol_TryAnotherProtocol) {
            ++conn->proto_idx;
        } else {
            break;
        }
    }
    if (conn->proto_idx >= _protocols.size()) {
        LOG_WARNING << "PbServer can't parse with all protocol";
        conn->close();
        return ;
    }
    if (res == ParseProtocol_Error) {
        LOG_WARNING << "PbServer can't parse with protocol " << _protocols[conn->proto_idx]->name();
        conn->close();
        return ;
    }
    if (res == ParseProtocol_NoEnoughData) {
        return ;
    }
    LOG_DEBUG << "PbServer get parse context: " << *conn->context();

    // 这里分包成功了, 开始调用 request_callback
    request_callback(conn);
}

// write_complete_callback
void PbServer::write_complete_callback(TCPConnection* conn) {
    LOG_DEBUG << "in write_complete_callback";
    ConnType tmp = conn->context()->conn_type();

    // clear ctx and buffer
    delete *conn->mutable_context();
    conn->set_context(nullptr);
    conn->rbuf().clear();
    conn->wbuf().clear();

    if (tmp == ConnType_Short) {
        close_callback(conn);
    } else {
        conn->set_event(EPOLLIN);
        conn->el()->update_channel(static_cast<Channel*>(conn));
        if (_read_timeout_ms > 0) {
            // 设置读超时定时器
            conn->timer_id.fetch_add(1);
            conn->el()->run_after(_read_timeout_ms, std::bind(&PbServer::timeout_callback,
                conn->el(), conn, conn->ts, conn->timer_id.load(), true));
        }
    }
}

// close_callback
void PbServer::close_callback(TCPConnection* conn) {
    LOG_DEBUG << "in close_callback";
    conn->set_event(0);
    conn->el()->update_channel(static_cast<Channel*>(conn));
    ::close(conn->fd());
    LOG_NOTICE << "disconnect from " << conn->fd();
    // 添加到channels
    conn->el()->remove_channel(conn, conn->ts);
    delete conn;
}

// request_callback
// message分包完毕
void PbServer::request_callback(TCPConnection* conn) {
    // 执行 应用层函数
    const std::string& pb_data = conn->context()->payload_view().str();
    const std::string& service_name = conn->context()->service();
    const std::string& method_name = conn->context()->method();
    LOG_DEBUG << "PbServer in request_callback: " << service_name << " " << method_name;

    ::google::protobuf::Service* service = nullptr;
    const ::google::protobuf::MethodDescriptor* method = nullptr;
    if (!_find_service_method(service_name, method_name, service, method)) {
        // 查找对应service method失败，关闭客户端
        close_callback(conn);
        return ;
    }

    // new message and parse request and prepare done
    google::protobuf::Message* request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(pb_data)) {
        delete request;
        // 解析失败 关闭客户端
        close_callback(conn);
        return ;
    }
    google::protobuf::Message* response = service->GetResponsePrototype(method).New();

    ReqRespConnPack* pack = new ReqRespConnPack(request, response, conn);

    ::google::protobuf::Closure* done = ::google::protobuf::NewCallback(this,
        &small_rpc::PbServer::response_callback, pack);

    // call method
    service->CallMethod(method, nullptr, request, response, done);
}

// response_callback
void PbServer::response_callback(ReqRespConnPack* pack) {
    std::string resp_str;
    assert(pack->_response->SerializeToString(&resp_str));
    delete pack->_request;
    delete pack->_response;
    TCPConnection* conn = pack->_conn;
    delete pack;

    (*conn->mutable_context())->set_rpc_status(RpcStatus_OK);
    *(*conn->mutable_context())->mutable_payload() = std::move(resp_str);
    bool ret = conn->context()->pack_response(conn->wbuf());
    if (!ret) { close_callback(conn); }

    conn->set_event(EPOLLOUT);
    // 同步调用
    // conn->el()->update_channel(static_cast<Channel*>(conn));
    // 异步调用 兼容同步
    conn->el()->add_func(std::bind(&EventLoop::update_channel, conn->el(), conn));

    // 设置写定时器
    if (_write_timeout_ms > 0) {
        // 设置读超时定时器
        conn->timer_id.fetch_add(1);
        conn->el()->run_after(_write_timeout_ms, std::bind(&PbServer::timeout_callback,
            conn->el(), conn, conn->ts, conn->timer_id.load(), false));
    }
}

// timeout_callback
void PbServer::timeout_callback(EventLoop* el, TCPConnection* conn,
        TimeStamp& conn_ts, int timer_id, bool is_read) {

    // 需要判断conn还在不在
    if (el->has_channel(conn, conn_ts)) {
        if (conn->timer_id.load() == timer_id) {
            LOG_WARNING << "PbServer handle " << (is_read ? "read" : "write")
                << " timeout. conn: " << conn;
            close_callback(conn);
        } else {
            LOG_DEBUG << "PbServer handle " << (is_read ? "read" : "write")
                << " timeout. unvalid timer. conn: " << conn;
        }
    } else {
        LOG_DEBUG << "PbServer handle " << (is_read ? "read" : "write")
            << " timeout. unvalid conn: " << conn;
    }
}

}; // namespace small_rpc
