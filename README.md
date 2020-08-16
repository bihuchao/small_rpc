# small_rpc
一个基于[Protobuf](https://developers.google.com/protocol-buffers)和[reactor模式](https://www.dre.vanderbilt.edu/~schmidt/PDF/reactor-siemens.pdf)的C++多线程网络编程框架。

### Features
* 采用**流式日志**API: 用户仅需实现`std::ostream& (std::ostream& os, const T& t);`即可轻松打印自定义类型, 支持日志分级、日志文件存在监测以及日志滚动。
* 服务端不同层级编程支持: a. 通过编写`Protobuf`接口定义文件, 编译得到的Server/Client Stub框架代码, 用户通过继承Stub接口更方便高效地编写实现业务代码, 服务端支持注册多Service/多Method;  b. 通过设定`TCPConnection`不同回调函数直接读写`Buffer`, 完成自定义分包与其余逻辑。
* 支持多种协议及自定义协议: 内置`SimpleProtocol` / `HTTP`协议, 服务端支持单端口多协议; 实现方面, 协议层与框架代码解耦, 继承基类对象`Context`/`Protocol`即可实现自定义协议, 协议实现见`protocols`。
* 支持长/短连接管理以及读写超时设定: 客户端/服务端均支持长/端连接管理, 支持同一个TCP连接上多个Method Call; 服务端支持设定读超时、写超时。
* 服务端支持同步/异步Service: 同步Service只需使用`DoneGuard`, 异步Service需要自行保存done对象并在异步调用结束后执行`done->Run()`。
* 客户端支持同步/异步调用: 客户端发起RPC交互支持同步调用方式, 支持自定义回调函数来异步调用。
* 构建工具及依赖: 采用CMake作为构建工具, 采用Gflags管理配置, 采用Gtest管理单元测试。

### Echo Example
1. 编写Protobuf接口定义文件, 以proto3作为范例
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
DEFINE_int32(read_timeout_ms, 2000, "read timeout ms");
DEFINE_int32(write_timeout_ms, 2000, "write timeout ms");
DEFINE_int32(server_thread_num, 2, "server thread num");

int main(int argc, char** argv) {
    // ... init gflags
    small_rpc::PbServer pb_server("0.0.0.0", 8878);
    // 支持单端口多协议
    assert(pb_server.set_protocol(new small_rpc::HTTPProtocol()));
    assert(pb_server.add_protocol(new small_rpc::SimpleProtocol()));
    // 支持多服务挂载
    assert(pb_server.add_service(new example::EchoServiceImpl()));
    // 支持读写超时 默认未开启
    pb_server.set_read_timeout_ms(FLAGS_read_timeout_ms);
    pb_server.set_write_timeout_ms(FLAGS_write_timeout_ms);
    // 支持多个eventloop线程 默认为1个主eventloop线程
    pb_server.set_thread_num(FLAGS_server_thread_num);
    pb_server.start();
    // ... 等待signal
    pb_server.stop();

    return 0;
}
```

4. 编写client逻辑
``` C++
DEFINE_bool(use_simple_protocol_rather_than_http_protocol, true,
    "use simple protocol rather than http protocol");
int main(int argc, char** argv) {
    // ... init gflags and signal
    small_rpc::PbClient client("127.0.0.1", 8878);
    assert(client.connected());
    // 支持不同协议
    if (FLAGS_use_simple_protocol_rather_than_http_protocol) {
        LOG_NOTICE << "client use simple protocol";
        assert(client.set_protocol(new small_rpc::SimpleProtocol()));
    } else {
        LOG_NOTICE << "client use http protocol";
        assert(client.set_protocol(new small_rpc::HTTPProtocol()));
    }
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
