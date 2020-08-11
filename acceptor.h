// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include "callbacks.h"
#include "channel.h"

namespace small_rpc {

class Acceptor : public Channel {
public:
    Acceptor(EventLoop* el, const char* addr, unsigned short port);
    ~Acceptor();

    void handle_events(int events);

    void handler_new_connection();

    void set_new_connection_callback(NewConnectionCallback new_connection_callback) {
        _new_connection_callback = new_connection_callback;
    }

private:
    NewConnectionCallback _new_connection_callback;
};

}; // namespace small_rpc
