# small_rpc
一个基于[Protobuf](https://developers.google.com/protocol-buffers)和[reactor模式](https://www.dre.vanderbilt.edu/~schmidt/PDF/reactor-siemens.pdf)的C++多线程网络编程框架。

### Features
* 采用**流式日志**，用户仅需实现`std::ostream& (std::ostream& os, const T& t);`即可轻松打印自定义类型。
* 支持在不同层级进行编程: 1) 通过Protobuf定义接口文件，借助Protobuf生成的server_stub / client_stub框架代码进行编程; 2) 也通过`TCPConnection`回调函数直接读写`Buffer`。
* 基于Protobuf接口编程模式支持多种协议: 内置`simple_protocol`、`HTTP`协议，server同时支持**单端口多协议**。
* 协议层支持与框架代码解耦: 通过继承`Context`和`Protocol`即可实现自定义协议，协议实现见`protocols`。
* 客户端/服务器均支持长/短连接: 客户端/服务端均支持长/端连接管理，支持同一个TCP连接上多个Method Call。
* 服务端支持同步/异步Service: 同步Service只需使用DoneGuard，异步Service需要自行保存done对象并在异步调用结束后执行done->Run()。
* 客户端支持同步/异步调用: 客户端发起RPC交互支持同步调用方式，支持自定义回调函数来异步调用。
* 基于Linux下`epoll`多路复用API实现。
* 采用gflags管理配置，采用gtest进行单元测试，便于开发测试与上线。

### TODO
* 读写超时支持

### Echo Example
1. 编写Protobuf接口定义文件，以proto3作为范例
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
2. 继承Protobuf生成代码Stub类来完成业务逻辑
``` C++
class EchoServiceImpl : public example::EchoService {
public:
    void echo(::google::protobuf::RpcController* controller,
            const ::example::EchoRequest* request,
            ::example::EchoResponse* response,
            ::google::protobuf::Closure* done) {

        small_rpc::DoneGuard done_guard(done);
        // 用户业务逻辑
        LOG_NOTICE << "enter EchoServiceImpl echo";
        LOG_DEBUG << "request: " << request->DebugString();
        response->set_logid(request->logid());
        response->set_result(request->message() + " powered by EchoService");
        LOG_NOTICE << "exit EchoServiceImpl echo";

        // 同步调用
        // 异步调用 需要手动自行保存 done对象 并调用done->Run();
        // done_guard.release();
    }
};
```
3. 编写server逻辑
``` C++
int main(int argc, char** argv) {
    // ... init gflags
    small_rpc::PbServer pb_server("0.0.0.0", 8878);
    // 支持单端口多协议
    assert(pb_server.set_protocol(new small_rpc::HTTPProtocol()));
    assert(pb_server.add_protocol(new small_rpc::SimpleProtocol()));
    // 支持多服务挂载
    assert(pb_server.add_service(new example::EchoServiceImpl()));
    pb_server.start();
    // ... 等待signal
    pb_server.stop();

    return 0;
}
```

4. 编写client逻辑
``` C++
int main(int argc, char** argv) {
    // ... init gflags and signal
    small_rpc::PbClient client("127.0.0.1", 8878);
    assert(client.connected());
    // 支持不同协议
    // assert(client.set_protocol(new small_rpc::SimpleProtocol()));
    assert(client.set_protocol(new small_rpc::HTTPProtocol()));
    small_rpc::PbController cntl;
    // 短连接
    cntl.set_conn_type(small_rpc::ConnType_Short);
    // 长连接example 参见 examples/echo/long_conn_client.cpp
    example::EchoService_Stub stub(&client);
    example::EchoRequest req;
    req.set_logid(1000);
    req.set_message("helloworld");
    example::EchoResponse resp;
    // 同步调用
    stub.echo(&cntl, &req, &resp, nullptr);
    // 异步调用example 参见 examples/echo/async_client.cpp
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
