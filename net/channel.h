// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <unistd.h>

namespace small_rpc {

class EventLoop;

// Channel
class Channel {
public:
    Channel(int fd = -1, EventLoop* el = nullptr)
        : _fd(fd), _event(0), _sevent(0), _el(el) {}

    virtual ~Channel() {
        if (_fd != -1) {
            ::close(_fd);
            _fd = -1;
        }
    }

    int fd() const { return _fd; }
    EventLoop* el() const { return _el; }
    int event() const { return _event; }
    int sevent() const { return _sevent; }
    void set_event(int event) { _event = event; }
    void set_sevent(int sevent) { _sevent = sevent; }

    virtual void handle_events(int events) = 0;

protected:
    int _fd;
    int _event, _sevent;
    EventLoop* _el;
};

}; // namespace small_rpc
