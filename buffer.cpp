// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "buffer.h"
#include "logging.h"
#include <unistd.h>

namespace small_rpc {

const char* ReadBuffer::CRLF = "\r\n";

// socket => read_buffer
// -1 : eagain
//  0 : eof
// >0 : data
int ReadBuffer::read_fd(int fd) {
    if (writeable() == 0) {
        _data.resize(_data.size() * 2);
    }
    int n = read(fd, end(), writeable());
    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1;
        } else {
            PLOG_FATAL << "failed to invoke read";
        }
    } else {
        _windex += n;
    }
    return n;
}

// find_crlf in read_buffer
// -1  : not found
// >=0 : found \r\n
int ReadBuffer::find_crlf(const size_t& len) {

    void* ret = memmem(static_cast<const void*>(begin()), len,
        static_cast<const void*>(CRLF), CRLFLen);
    if (ret == nullptr) { return -1; }

    return static_cast<const char*>(ret) - begin();
}

// write_buffer => socket
int WriteBuffer::write_fd(int fd) {
    int total = 0;
    if (readable()) {
        int n = write(fd, begin(), readable());
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return -1;
            } else {
                PLOG_FATAL << "failed to invoke write";
            }
        } else {
            _rindex += n;
            total += n;
        }
    }
    if (extraable()) {
        int n = write(fd, extra_begin(), extraable());
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return -1;
            } else {
                PLOG_FATAL << "failed to invoke write";
            }
        } else {
            _extra_index += n;
            total += n;
        }
    }
    return total;
}

}; // namespace small_rpc
