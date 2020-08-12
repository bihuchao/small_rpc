// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <string.h>
#include <vector>
#include <string>
#include <arpa/inet.h>
#include <ostream>

namespace small_rpc {

// Buffer
class Buffer {
public:
    Buffer(size_t buf_size) : _rindex(0), _windex(0), _data(buf_size), _extra_index(0) {}
    Buffer() : _rindex(0), _windex(0), _data(InitialBufferSize), _extra_index(0) {}
    virtual ~Buffer() {}

    // status member functions
    size_t readable() const { return _windex - _rindex; }
    size_t writeable() const { return _data.size() - _windex; }
    char* begin() { return &_data[_rindex]; }
    char* end() { return &_data[_windex]; }
    const char* data() const { return &_data[0]; }
    // extra data
    size_t extraable() const { return _extra_data.size() - _extra_index; }
    const char* extra_begin() const { return &_extra_data[_extra_index]; }

    // peek member functions
    int32_t peek_int32() {
        int32_t* tmp = reinterpret_cast<int32_t*>(&_data[_rindex]);
        int32_t num = ntohl(*tmp);
        return num;
    }

    uint32_t peek_uint32() {
        uint32_t* tmp = reinterpret_cast<uint32_t*>(&_data[_rindex]);
        uint32_t num = ntohl(*tmp);
        return num;
    }

    // find crlf
    int find_crlf(const size_t& len);

    // read member functions
    // retrieve data
    void retrieve(size_t n) { _rindex += n; }
    // mem => socket
    int write_fd(int fd);

    // write member functions
    // socket => buffer
    int read_fd(int fd);

    // append
    void append(const char* str) {
        _append(str, strlen(str));
    }

    void append(const std::string& str) {
        _append(str.data(), str.length());
    }

    void append(int32_t num) {
        int32_t numm = htonl(num);
        _append(&numm, sizeof(int32_t));
    }

    void append(uint32_t num) {
        uint32_t numm = htonl(num);
        _append(&numm, sizeof(uint32_t));
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

private:
    void _append(const void* mem, int memlen) {
        if (writeable() < memlen) { _data.resize(_data.size() * 2); }
        memcpy(&_data[_windex], mem, memlen);
        _windex += memlen;
    }

public:
    static const int InitialBufferSize = 1024;

protected:
    size_t _rindex, _windex;
    std::vector<char> _data;
    size_t _extra_index;
    std::string _extra_data;
friend class BufferView;
};

// BufferView
class BufferView {
public:
    BufferView() : _buf(nullptr), _begin(0), _end(0) {}

    BufferView(const Buffer& buf, size_t length)
        : _buf(&buf), _begin(_buf->_rindex), _end(_begin + length) {}

    BufferView(const Buffer* buf, size_t begin, size_t end) : _buf(buf), _begin(begin), _end(end) {}

    BufferView(const Buffer& buf, const char* begin, size_t length)
        : _buf(&buf), _begin(begin - _buf->data()), _end(_begin + length) {}

    bool empty() const { return _begin == _end; }
    size_t size() const { return _end - _begin; }

    std::string str() const {
        if (!_buf) { return std::string(); }
        return std::string(_buf->data() + _begin, _buf->data() + _end);
    }
private:
    const Buffer* _buf;
    size_t _begin, _end;
};

inline std::ostream& operator<<(std::ostream& os, const BufferView& bufview) {
    os << bufview.str();
    return os;
}

}; // namespace small_rpc
