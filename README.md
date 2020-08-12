# small_rpc
一个基于[Protobuf](https://developers.google.com/protocol-buffers)和[reactor模式](https://www.dre.vanderbilt.edu/~schmidt/PDF/reactor-siemens.pdf)的C++多线程网络编程框架。

### Features
* 采用**流式日志**，用户仅需实现`std::ostream& (std::ostream& os, const T& t);`即可轻松打印自定义类型。
* 支持在不同层级进行编程: 1) 通过Protobuf定义接口文件，借助Protobuf生成的server_stub / client_stub框架代码进行编程; 2) 也通过`TCPConnection`回调函数直接读写`Buffer`。
* 基于Protobuf接口编程模式支持多种协议: 内置`simple_protocol`、`HTTP`协议，server同时支持**单端口多协议**。
* 协议层支持与框架代码解耦: 通过继承`Context`和`Protocol`即可实现自定义协议，协议实现见`protocols`。
* 基于Linux下`epoll`多路复用API。
* 采用gflags管理配置，采用gtest进行单元测试，便于开发测试与上线。

### Echo Example
1. 首先定义proto文件，以proto3作为范例
``` protobuf
syntax = "proto3";

option cc_generic_services = true;

package example;

message EchoRequest {
    uint64 logid   = 1;
    bytes  message = 2;
};

message EchoResponse {
    uint64 logid  = 1;
    bytes  result = 2;
};

service EchoService {
    rpc echo(EchoRequest) returns (EchoResponse);
};

```
2. 其次继承Protobuf生成代码Stub类来完成业务逻辑
``` C++
class EchoServiceImpl : public example::EchoService {
public:
    void echo(::google::protobuf::RpcController* controller,
            const ::example::EchoRequest* request,
            ::example::EchoResponse* response,
            ::google::protobuf::Closure* done) {
        // 用户业务逻辑
        LOG_NOTICE << "enter EchoServiceImpl echo";
        LOG_DEBUG << "request: " << request->DebugString();
        response->set_logid(request->logid());
        response->set_result(request->message() + " powered by EchoService");
        LOG_NOTICE << "exit EchoServiceImpl echo";
        done->Run();
    }
};
```
3. 编写server逻辑
``` C++
int main(int argc, char** argv) {
    // ... init gflags
    small_rpc::PbServer pb_server("0.0.0.0", 8878);
    assert(pb_server.add_protocol(new small_rpc::SimpleProtocol()));
    assert(pb_server.add_service(new example::EchoServiceImpl()));
    pb_server.start();
    // ... 等待signal
    pb_server.stop();

    return 0;
}
```

### Build And Run
```
mkdir build && cd build
cmake .. -DProtobuf_DIR={your_protobuf_path}
make -j
./examples/echo_server/echo_server >console.txt 2>&1 &
./examples/echo_server/echo_client
```
