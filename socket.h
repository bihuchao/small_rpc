// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <ostream>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <fcntl.h>

namespace small_rpc {

std::ostream& operator<<(std::ostream& os, struct sockaddr_in addr);

struct sockaddr_in get_addr(const char* addr, unsigned short port);

void set_nonblocking(int fd);

int listen_socket(const char* addr, unsigned short port, int backlog = 1024);

}; // namespace mall_rpc
