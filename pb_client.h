// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "channel.h"
#include <google/protobuf/service.h>

namespace small_rpc {

// PbClient
class PbClient : public Channel, public ::google::protobuf::RpcChannel {
public:

    // PbClient
    PbClient(const char* addr, unsigned short port);

    // ~PbClient
    virtual ~PbClient();

    // handle_events
    void handle_events(int events);

    // CallMethod
    void CallMethod(const ::google::protobuf::MethodDescriptor* method,
            ::google::protobuf::RpcController* cntl,
            const ::google::protobuf::Message* request, ::google::protobuf::Message* response, ::google::protobuf::Closure* done);

private:
    int _fd;

};

}; // namespace small_rpc
