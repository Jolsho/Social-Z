#pragma once 
#include "http/request.h"
#include "llhttp.h"
#include "utils/lru.h"
#include "openssl/ssl.h"
#include <queue>

class conn_t {
public:
    std::string     ip;
    ConnID          id  = 0;
    int             fd  = -1;
    SSL*            ssl = nullptr;
    int             hand_failures = 0;
    bool            is_ws   = false;
    uint32_t        events  = 0;
    Status::Error   err     = Status::OK_E;
    ConnNode*       lru_node = nullptr;
    // TODO -- status


    //  OUTBOUND  //
    std::queue<int>         res_q;
    std::vector<msg::Msg*>  outbound_msgs;
    Buffer<MX_HEADER_LEN>   out_h {};
    bool                    written_h = false;
    Buffer<MX_BODY_LEN>     outbuf {};

    //  INCOMING //
    llhttp_t                parser;
    Buffer<MX_BODY_LEN>     inbuf {};
    Request                 r {};

    conn_t() {
        lru_node = new ConnNode;
    }
    ~conn_t() {
        if (lru_node) delete lru_node;
    }

    int write_();
    int read_();
    int drive_tls_handshake();
    int parse_ws(size_t nread);

    void queue_err();
    int queue_response(msg::Msg* msg);

    inline bool has_outgoing() {
        return outbuf.cursor != 0 || 
            out_h.cursor != 0 || 
            outbound_msgs.size() > 0;
    }

    void wipe() {
        // TODO DOUBLE CHECK
        fd = -1;

        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;

        hand_failures = 0;

        r.clear();

        inbuf.cursor = 0;
        outbuf.cursor = 0;
        written_h = false;
    }
};
