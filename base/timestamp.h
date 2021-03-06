// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <ostream>

namespace small_rpc {

// TimeStamp
class TimeStamp {
public:
    TimeStamp() : _sec(0), _msec(0) {}
    TimeStamp(long sec, long msec) : _sec(sec), _msec(msec) {}
    long sec() const { return _sec; }
    long msec() const { return _msec; }

    std::string str() const {
        char buffer[16];
        struct tm result;
        localtime_r(static_cast<const time_t*>(&_sec), &result);
        // example: 20200101100000
        snprintf(buffer, 16, "%04d%02d%02d_%02d%02d%02d",
            1900 + result.tm_year, 1 + result.tm_mon, result.tm_mday,
            result.tm_hour, result.tm_min, result.tm_sec);
        return buffer;
    }

    bool operator== (const TimeStamp& that) const {
        return _sec == that._sec && _msec == that._msec;
    }

    bool operator< (const TimeStamp& that) const {
        if (_sec != that._sec) {
            return _sec < that._sec;
        }
        return _msec < that._msec;
    }

    int operator- (const TimeStamp& that) const {
        return static_cast<int>((_sec - that._sec) * 1000 + _msec - that._msec);
    }

public:
    static TimeStamp now() {
        struct timeval tv;
        ::gettimeofday(&tv, nullptr);
        long sec = tv.tv_sec;
        long msec = tv.tv_usec / 1000;
        return TimeStamp(sec, msec);
    }

    static TimeStamp after_now_ms(long after_time_ms) {
        TimeStamp ts = now();
        long tmp = ts._msec + after_time_ms;
        ts._sec += tmp / 1000;
        ts._msec = tmp % 1000;
        return ts;
    }

private:
    long _sec;
    long _msec;
friend std::ostream& operator<<(std::ostream& os, const TimeStamp& ts);
};

inline std::ostream& operator<<(std::ostream& os, const TimeStamp& ts) {
    char buffer[32];
    struct tm result;
    localtime_r(static_cast<const time_t*>(&ts._sec), &result);
    // example: 2020-01-01 10:00:00:000
    snprintf(buffer, 32, "%04d-%02d-%02d %02d:%02d:%02d:%03ld",
        1900 + result.tm_year, 1 + result.tm_mon, result.tm_mday,
        result.tm_hour, result.tm_min, result.tm_sec, ts._msec);
    os << buffer;
    return os;
}

}; // namespace small_rpc
