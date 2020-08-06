// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <string.h>
#include <vector>
#include <string>

namespace small_rpc {

// Buffer
class Buffer {
public:
    Buffer(size_t buf_size) : _rindex(0), _windex(0), _data(buf_size) {}
    Buffer() : _rindex(0), _windex(0), _data(InitialBufferSize) {}
    virtual ~Buffer() {}
    size_t readable() const { return _windex - _rindex; }
    size_t writeable() const { return _data.size() - _windex; }
    char* begin() { return &_data[_rindex]; }
    char* end() { return &_data[_windex]; }
    const char* data() const { return &_data[0]; }

public:
    static const int InitialBufferSize = 1024;

protected:
    size_t _rindex, _windex;
    std::vector<char> _data;
};

// BufferView
class BufferView {
public:
    BufferView() : _buf(nullptr), _begin(0), _end(0) {}

    BufferView(const Buffer* buf, size_t begin, size_t end) : _buf(buf), _begin(begin), _end(end) {}

    BufferView(const Buffer& buf, const char* begin, size_t length)
        : _buf(&buf), _begin(begin - _buf->data()), _end(_begin + length) {}

    std::string str() const {
        if (!_buf) { return std::string(); }
        return std::string(_buf->data() + _begin, _buf->data() + _end);
    }
private:
    const Buffer* _buf;
    size_t _begin, _end;
};

// ReadBuffer
class ReadBuffer : public Buffer {
public:
    ~ReadBuffer() {}

    // socket => buffer
    int read_fd(int fd);

    // retrieve data
    void retrieve(size_t n) { _rindex += n; }

    // find crlf
    int find_crlf(const size_t& len);

public:
    static const char* CRLF; // "\r\n"
    static const size_t CRLFLen = 2;
};

// WriteBuffer
class WriteBuffer : public Buffer {
public:
    WriteBuffer() : _extra_index(0) {}
    ~WriteBuffer() {}

    // mem => socket
    int write_fd(int fd);

    // append
    void append(const char* str) {
        append(str, strlen(str));
    }

    void append(const std::string& str) {
        append(str.data(), str.length());
    }

    void append(const char* str, int len) {
        strncpy(&_data[_windex], str, len);
        _windex += len;
    }

    // append_extra
    void append_extra(const std::string& extra_data) {
        _extra_data.append(extra_data);
    }

    void append_extra(std::string&& extra_data) {
        if (_extra_data.empty()) {
            _extra_data.swap(extra_data);
        } else {
            _extra_data.append(extra_data);
        }
    }

    // extra data
    size_t extraable() const { return _extra_data.size() - _extra_index; }
    const char* extra_begin() const { return &_extra_data[_extra_index]; }

private:
    size_t _extra_index;
    std::string _extra_data;
};

}; // namespace small_rpc
