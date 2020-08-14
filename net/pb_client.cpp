// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "pb_client.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include "base/logging.h"
#include "protocols/protocol.h"
#include "net/socket.h"
#include "net/buffer.h"
#include "net/eventloop.h"

namespace small_rpc {

// PbClient
// TODO 支持长连接
// TODO 支持多协议
PbClient::PbClient(const char* addr, unsigned short port)
        : PbClient(nullptr, addr, port) {
}

// PbClient
PbClient::PbClient(EventLoop* el, const char* addr, unsigned short port)
        : TCPConnection(-1, el, false), _protocol(nullptr) , _donee(false) {

    _connected.store(false);
    _server_addr = get_addr(addr, port);
    _fd = ::socket(AF_INET, SOCK_STREAM, 0);
    PLOG_FATAL_IF(_fd == -1) << "PbClient failed to invoke ::socket";
    if (_el) {
        // 异步
        small_rpc::set_nonblocking(_fd);

        set_close_callback(std::bind(&PbClient::close_callback, this));
        set_data_read_callback(std::bind(&PbClient::data_read_callback, this));
        set_write_complete_callback(std::bind(&PbClient::write_complete_callback, this));
        set_client_conn_callback(std::bind(&PbClient::client_conn_callback, this));

        int err = ::connect(_fd, reinterpret_cast<struct sockaddr*>(&_server_addr),
            sizeof(_server_addr));
        if (err == -1) {
            if (errno == EINPROGRESS) {
                _event = EPOLLOUT;
                _el->add_func(std::bind(&EventLoop::update_channel, _el, this));
            } else {
                PLOG_FATAL << "PbClient failed to invoke ::connect " << _server_addr;
            }
        } else {
            _connected.store(true);
        }
    } else {
        // 同步
        int err = ::connect(_fd, reinterpret_cast<struct sockaddr*>(&_server_addr),
            sizeof(_server_addr));
        PLOG_FATAL_IF(err == -1) << "PbClient failed to invoke ::connect " << _server_addr;
        _connected.store(true);
    }
}

// set_protocol
bool PbClient::set_protocol(Protocol* protocol) {
    if (_protocol) {
        delete _protocol;
        _protocol = nullptr;
    }
    _protocol = protocol;
    return true;
}

// client_conn_callback
void PbClient::client_conn_callback() {
    int so_error = 0;
    socklen_t so_error_len = sizeof(so_error);
    int err = ::getsockopt(_fd, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len);
    PLOG_FATAL_IF(err == -1) << "PbClient failed to invoke ::getsockopt to "
        << "get SOL_SOCKET SO_ERROR";
    if (so_error == 0) {
        _connected.store(true);
    } else {
        LOG_WARNING << "PbClient can't connect server with error: " << so_error;

    }
    _event = 0;
    _el->update_channel(this);
}

// read_callback
void PbClient::data_read_callback() {
    ParseProtocolStatus ret = _ctx->parse_response(_rbuf);
    if (ret == ParseProtocol_Error) {
        LOG_WARNING << "PbClient ParseProtocol_Error";
        return ;
    }
    if (ret == ParseProtocol_Success) {
        response_callback();
    }
}

// response_callback
void PbClient::response_callback() {
    std::string str = _ctx->payload_view().str();
    // TODO remove class member
    _response->ParseFromString(str);
    if (_done) { _done->Run(); }
    _donee = true;
    close_callback();
}

// write_complete_callback
void PbClient::write_complete_callback() {
    _event = EPOLLIN;
    _el->update_channel(this);
}

// close_callback
void PbClient::close_callback() {
    _event = 0;
    _el->update_channel(this);
}

// CallMethod
void PbClient::CallMethod(const ::google::protobuf::MethodDescriptor* method,
        ::google::protobuf::RpcController* cntl, const ::google::protobuf::Message* request,
        ::google::protobuf::Message* response, ::google::protobuf::Closure* done) {

    _ctx = _protocol->new_context();
    _ctx->set_conn_type(ConnType_Short);
    _ctx->set_method(method->name());
    _ctx->set_service(method->service()->name());
    request->SerializeToString(_ctx->mutable_payload());
    if (_ctx->payload().length() > UINT32_MAX) {
        LOG_WARNING << "PbClient req_str length beyond UINT32_MAX";
        return;
    }
    _ctx->pack_request(_wbuf);
    _response = response;
    _done = done;

    if (done && _el) {
        // 异步
        _event = EPOLLOUT;
        _el->add_func(std::bind(&EventLoop::update_channel, _el, this));
        LOG_NOTICE << "PbClient async end";
    } else {
        // 同步
        while (_wbuf.readable() || _wbuf.extraable()) {
            _wbuf.write_fd(_fd);
        }
        while (!_donee) {
            _rbuf.read_fd(_fd);
            data_read_callback();
        }
        LOG_NOTICE << "PbClient sync end";
    }
}

}; // namespace small_rpc
