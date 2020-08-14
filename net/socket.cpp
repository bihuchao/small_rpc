// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "socket.h"
#include "base/logging.h"

namespace small_rpc {

std::ostream& operator<<(std::ostream& os, struct sockaddr_in addr) {
    char buf[INET_ADDRSTRLEN] = {"\0"};
    inet_ntop(AF_INET, &addr.sin_addr, buf, INET_ADDRSTRLEN);
    os << buf << ":" << ntohs(addr.sin_port);
    return os;
}

// get_addr
struct sockaddr_in get_addr(const char* addr, unsigned short port) {
    // get addr
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = ::htons(port);
    int err = ::inet_pton(AF_INET, addr, &server_addr.sin_addr);
    PLOG_FATAL_IF(err != 1) << "failed to invoke ::inet_pton";
    return server_addr;
}

// set_nonblocking
void set_nonblocking(int fd) {
    int flags = ::fcntl(fd, F_GETFL);
    int err = ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    PLOG_FATAL_IF(err == -1) << "failed to invoke ::fcntl";
}

// listen_socket
int listen_socket(const char* addr, unsigned short port, int backlog) {
    // get socket and set nonblock
    int listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    PLOG_FATAL_IF(listen_fd == -1) << "failed to invoke ::socket";
    set_nonblocking(listen_fd);

    // setsockopt
    int option = 1;
    int err = ::setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&option, sizeof(option));
    PLOG_FATAL_IF(err == -1) << "failed to invoke ::setsockopt SO_REUSEADDR";

    // bind
    struct sockaddr_in server_addr = get_addr(addr, port);
    err = ::bind(listen_fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
    PLOG_FATAL_IF(err == -1) << "failed to invoke ::bind. addr: " << server_addr;

    // listen
    err = ::listen(listen_fd, backlog);
    PLOG_FATAL_IF(err == -1) << "failed to invoke ::listen. addr: " << server_addr;

    return listen_fd;
}

}; // namespace small_rpc
