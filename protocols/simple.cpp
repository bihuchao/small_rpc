// Use of this source code is governed by a BSD-style license.
//
// Author: Huchao Bi (bihuchao at qq dot com)

#include "simple.h"
#include "base/logging.h"

namespace small_rpc {

// parse_request
ParseProtocolStatus SimpleContext::parse_request(Buffer& rd_buf) {
    while (true) {
        if (_stage == 0) {
            // parse headers
            if (rd_buf.readable() < sizeof(int)) {
                return ParseProtocol_NoEnoughData;
            }
            // magic_num
            int magic_num = rd_buf.peek_int32();
            if (magic_num != SimpleProtocol::MAGIC_NUM) {
                LOG_WARNING << "SimpleContext invalid magic_num: " << magic_num;
                return ParseProtocol_Error;
            }
            rd_buf.retrieve(sizeof(int));

            _stage = 1;
        } else if (_stage == 1) {
            // conn_type
            if (rd_buf.readable() < sizeof(int)) {
                return ParseProtocol_NoEnoughData;
            }
            int conn_type = rd_buf.peek_int32();
            if (!ConnType_IsValid(conn_type)) {
                LOG_WARNING << "SimpleContext invalid conn_type: " << _conn_type;
                return ParseProtocol_Error;
            }
            rd_buf.retrieve(sizeof(int));
            _conn_type = ConnType(conn_type);

            _stage = 2;
        } else if (_stage == 2) {
            // parse path
            if (rd_buf.readable() < sizeof(uint32_t)) {
                return ParseProtocol_NoEnoughData;
            }
            // path_len
            uint32_t path_len = rd_buf.peek_uint32();
            if (rd_buf.readable() < path_len) {
                return ParseProtocol_NoEnoughData;
            }
            rd_buf.retrieve(sizeof(uint32_t));

            const char* tmp = static_cast<const char*>(
                memchr(rd_buf.begin(), '/', path_len));
            if (!tmp) {
                LOG_WARNING << "SimpleContext failed to parse path";
                return ParseProtocol_Error;
            }

            _service = std::string(rd_buf.begin(), tmp - rd_buf.begin());
            _method = std::string(tmp + 1, path_len  - 1 - (tmp - rd_buf.begin()));
            rd_buf.retrieve(path_len);
            _stage = 3;
        } else if (_stage == 3) {
            // parse path
            if (rd_buf.readable() < sizeof(uint32_t)) {
                return ParseProtocol_NoEnoughData;
            }
            // path_len
            uint32_t payload_len = rd_buf.peek_uint32();
            if (rd_buf.readable() < payload_len) {
                return ParseProtocol_NoEnoughData;
            }
            rd_buf.retrieve(sizeof(uint32_t));
            _payload_view = BufferView(rd_buf, payload_len);
            _stage = 4;
            rd_buf.retrieve(payload_len);
        } else {
            break;
        }
    }
    return ParseProtocol_Success;
}

// pack_response
bool SimpleContext::pack_response(Buffer& wr_buf) const {
    wr_buf.append(SimpleProtocol::MAGIC_NUM);
    wr_buf.append(_conn_type);
    wr_buf.append(_rpc_status);
    uint32_t payload_len = _payload.length();
    wr_buf.append(payload_len);
    wr_buf.append_extra(std::move(_payload));

    return true;
}

// pack_request
bool SimpleContext::pack_request(Buffer& wr_buf) const {
    wr_buf.append(SimpleProtocol::MAGIC_NUM);
    wr_buf.append(_conn_type);

    uint32_t path_len = _service.length() + 1 + _method.length();
    wr_buf.append(path_len);
    wr_buf.append(_service);
    wr_buf.append("/");
    wr_buf.append(_method);

    uint32_t payload_len = _payload.length();
    wr_buf.append(payload_len);
    wr_buf.append_extra(std::move(_payload));

    return true;
}

// parse_response
ParseProtocolStatus SimpleContext::parse_response(Buffer& rd_buf) {
    while (true) {
        if (_stage == 0) {
            // parse headers
            if (rd_buf.readable() < sizeof(int)) {
                return ParseProtocol_NoEnoughData;
            }
            // magic_num
            int magic_num = rd_buf.peek_int32();
            if (magic_num != SimpleProtocol::MAGIC_NUM) {
                LOG_WARNING << "SimpleContext invalid magic_num: " << magic_num;
                return ParseProtocol_Error;
            }
            rd_buf.retrieve(sizeof(int));

            _stage = 1;
        } else if (_stage == 1) {
            // conn_type
            if (rd_buf.readable() < 2 * sizeof(int)) {
                return ParseProtocol_NoEnoughData;
            }
            int conn_type = rd_buf.peek_int32();
            if (!ConnType_IsValid(conn_type)) {
                LOG_WARNING << "SimpleContext invalid conn_type: " << _conn_type;
                return ParseProtocol_Error;
            }
            rd_buf.retrieve(sizeof(int));
            _conn_type = ConnType(conn_type);

            // status
            int rpc_status = rd_buf.peek_int32();
            if (!RpcStatus_IsValid(rpc_status)) {
                LOG_WARNING << "SimpleContext invalid rpc_status: " << rpc_status;
                return ParseProtocol_Error;
            }
            rd_buf.retrieve(sizeof(int));
            _rpc_status = RpcStatus(rpc_status);

            _stage = 2;
        } else if (_stage == 2) {
            // parse payload
            if (rd_buf.readable() < sizeof(uint32_t)) {
                return ParseProtocol_NoEnoughData;
            }
            // payload_len
            uint32_t payload_len = rd_buf.peek_uint32();
            if (rd_buf.readable() < payload_len) {
                return ParseProtocol_NoEnoughData;
            }
            rd_buf.retrieve(sizeof(uint32_t));
            _payload_view = BufferView(rd_buf, payload_len);
            _stage = 3;
            rd_buf.retrieve(payload_len);
        } else {
            break;
        }
    }
    return ParseProtocol_Success;
}

std::ostream& SimpleContext::print(std::ostream& os) const {
    os << "SimpleContext. service: " << _service
        << ", method: " << _method
        << ", payload_size: " << _payload_view.size();
    return os;
}

// parse_request
ParseProtocolStatus SimpleProtocol::parse_request(Buffer& rd_buf, Context** ctxx) {
    if (!ctxx) { return ParseProtocol_Error; }
    if (*ctxx == nullptr) {
        if (rd_buf.readable() < sizeof(MAGIC_NUM)) { return ParseProtocol_NoEnoughData; }
        if (rd_buf.peek_int32() != MAGIC_NUM) { return ParseProtocol_TryAnotherProtocol; }
        *ctxx = new SimpleContext();
    }
    SimpleContext* ctx = dynamic_cast<SimpleContext*>(*ctxx);
    if (!ctx) {
        LOG_WARNING << "SimpleProtocol failed to dynamic_cast SimpleContext.";
        return ParseProtocol_Error;
    }
    return ctx->parse_request(rd_buf);
}

}; //namespace small_rpc
