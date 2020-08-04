// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

namespace small_rpc {

class EventLoop;

class Channel {
public:
    Channel(int fd = -1, EventLoop* el = nullptr)
        : _fd(fd), _event(0), _sevent(0), _el(el) {}
    int fd() const { return _fd; }
    int event() const { return _event; }
    int sevent() const { return _sevent; }
    void set_event(int event) { _event = event; }
    void set_sevent(int sevent) { _sevent = sevent; }
    virtual void handle_events(int events) = 0;
    virtual ~Channel() {}
protected:
    int _fd;
    int _event, _sevent;
    EventLoop* _el;
};

}; // namespace small_rpc
