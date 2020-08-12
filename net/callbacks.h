// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <functional>

namespace small_rpc {

class TCPConnection;

// Acceptor
// 有新连接进来
using NewConnectionCallback = std::function<void (int)>;

// TCPConnection
// rd_buf 有数据进来
using DataReadCallback = std::function<void (TCPConnection*)>;
// wr_buf 数据写完
using DataWriteCompleteCallback = std::function<void (TCPConnection*)>;
// 对端关闭连接
using ConnectionCloseCallback = std::function<void (TCPConnection*)>;

}; // namespace small_rpc
