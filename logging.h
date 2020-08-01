// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#pragma once

#include <memory>
#include <string>
#include <ostream>
#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include "timestamp.h"

namespace small_rpc {

const int DEBUG = 0;
const int NOTICE = 1;
const int WARNING = 2;
const int FATAL = 3;

// 借鉴了brpc中相关设计思路
class _LogStream : public std::stringbuf, public std::ostream{
public:
    _LogStream() : std::ostream(static_cast<std::stringbuf*>(this)) {}
    const char* pbase() { return std::stringbuf::pbase(); }
    const char* pptr() { return std::stringbuf::pptr(); }
};

class _LogAppender {
public:
    virtual void append(const std::shared_ptr<_LogStream>& ls) = 0;
    virtual ~_LogAppender() {}
};

class _LogStdoutAppender : public _LogAppender {
public:
    void append(const std::shared_ptr<_LogStream>& ls) {
        std::cout << ls->str();
    };
    virtual ~_LogStdoutAppender() {}
};

class _LogFileAppender : public _LogAppender {
public:
    _LogFileAppender() : _f(nullptr), _file_node(0), _rollover_s(-1), _t(TimeStamp::now()) {}

    virtual bool init(const char* logfile, int rollover_min) {
        _filename = logfile;
        _rollover_s = rollover_min * 60;
        return open();
    }

    void append(const std::shared_ptr<_LogStream>& ls) {
        do_rollover();
        int len = ls->pptr() - ls->pbase();
        // fwrite是线程安全的, write不是线程安全的
        int ret = fwrite(ls->pbase(), 1, len, _f);
        fflush(_f);
        if (ret < len) {
            fprintf(stderr, "failed to append\n");
            close();
            open();
        }
    };

    bool do_rollover() {
        TimeStamp now = TimeStamp::now();
        if (_rollover_s > 0 && (now.sec() - _t.sec()) >= _rollover_s) {
            close();
            std::string bk_filename = _filename + "." + _t.str();
            rename(_filename.c_str(), bk_filename.c_str());
            open();
            _t = now;
        } else {
            stat(_filename.c_str(), &_file_stat);
            unsigned long inode = _file_stat.st_ino;
            if (_file_node != inode) {
                close();
                open();
                _t = now;
            }
        }
        return true;
    }

    bool open() {
        _f = fopen(_filename.c_str(), "a");
        if (_f == nullptr) {
            fprintf(stderr, "failed to open %s\n", _filename.c_str());
            return false;
        }
        stat(_filename.c_str(), &_file_stat);
        _file_node = _file_stat.st_ino;
        return true;
    }

    void close() {
        if (_f) {
            fclose(_f);
            _f = nullptr;
        }
    }

    virtual ~_LogFileAppender() {
        close();
    }
private:
    FILE* _f;
    unsigned long _file_node;
    int _rollover_s;
    TimeStamp _t;
    std::string _filename;
    struct stat _file_stat;
};

class _Logger {
public:
    _Logger() : _severity(small_rpc::DEBUG), _appender(
        std::shared_ptr<_LogAppender>(new _LogStdoutAppender())) {}
    int severity() const { return _severity; }
    bool set_severity(int severity) {
        if (severity >= small_rpc::DEBUG && severity <= small_rpc::FATAL) {
            _severity = severity;
            return true;
        }
        fprintf(stderr, "failed to set severity to %d\n", severity);
        return false;
    }
    void append(const std::shared_ptr<_LogStream>& ls) { _appender->append(ls); }
    bool set_appender(const std::shared_ptr<_LogAppender>& la) {
        if (la != nullptr) {
            _appender = la;
            return true;
        }
        fprintf(stderr, "failed to set appender to nullptr\n");
        return false;
    }
private:
    int _severity;
    std::shared_ptr<_LogAppender> _appender;
};

extern _Logger _logger;

class _LogRecord {
public:
    _LogRecord(const char* file, int lineno, int severity) : _stream(std::make_shared<_LogStream>()) {
        const char* severity_map[] = {
            "DEBUG", "NOTICE", "WARNING", "FATAL"
        };
        stream() << severity_map[severity] << ": "
            << TimeStamp::now() << " * "
            << getpid() << " * "
            << pthread_self() << " "
            << "[" << file << ":" << lineno << "]: ";
    }
    _LogStream& stream() { return *_stream; }
    ~_LogRecord() {
        stream() << "\n";
        small_rpc::_logger.append(_stream);
    }
private:
    std::shared_ptr<_LogStream> _stream;
};

inline bool set_severity(int severity) { return _logger.set_severity(severity); }

inline bool init_log(const char* logfile, int severity, int rollover_min = 30) {
    if (!set_severity(severity)) { return false; }
    std::shared_ptr<_LogFileAppender> appender = std::make_shared<_LogFileAppender>();
    if (appender == nullptr || !appender->init(logfile, rollover_min)) {
        return false;
    }
    return _logger.set_appender(appender);
}

}; // namespace small_rpc

#define LOG_DEBUG if (small_rpc::_logger.severity() <= small_rpc::DEBUG) \
    small_rpc::_LogRecord(__FILE__, __LINE__, small_rpc::DEBUG).stream()

#define LOG_NOTICE if (small_rpc::_logger.severity() <= small_rpc::NOTICE) \
    small_rpc::_LogRecord(__FILE__, __LINE__, small_rpc::NOTICE).stream()

#define LOG_WARNING if (small_rpc::_logger.severity() <= small_rpc::WARNING) \
    small_rpc::_LogRecord(__FILE__, __LINE__, small_rpc::WARNING).stream()

#define LOG_FATAL if (small_rpc::_logger.severity() <= small_rpc::FATAL) \
    small_rpc::_LogRecord(__FILE__, __LINE__, small_rpc::FATAL).stream()
