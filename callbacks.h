// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <functional>

namespace small_rpc {

class TCPConnection;

// acceptor有连接进来
using NewConnectionCallback = std::function<void (TCPConnection*)>;
// tcp_connection有数据进来
using DataReadCallback = std::function<void (TCPConnection*)>;
// message分包完毕
using RequestCallback = std::function<void (TCPConnection*)>;
// writebuffer数据写完
using WriteCompleteCallback = std::function<void (TCPConnection*)>;

}; // namespace small_rpc
