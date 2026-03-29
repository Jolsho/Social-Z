#pragma once 
#include "msg.h"
#include "http/request.h"
#include "llhttp.h"
#include "openssl/crypto.h"
#include "openssl/ssl.h"
#include <queue>
#include <string>



class conn_t {
public:
    std::string     ip;
    ConnID          id;
    int             fd;
    SSL*            ssl;
    int             hand_failures;
    bool            is_ws;
    uint32_t        events;
    Status::Error   err;


    //  OUTBOUND  //
    std::queue<int>         res_q;
    std::vector<msg::Msg*>  outbound_msgs;
    Buffer<MX_HEADER_LEN>   out_h;
    Buffer<MX_BODY_LEN>     outbuf;


    //  INCOMING //
    llhttp_t                parser;
    Buffer<MX_BODY_LEN>     inbuf;
    Request                 r {};

    int write_();
    int read_();
    int drive_tls_handshake();
    int parse_ws(size_t nread);
    int queue_response(
        std::string_view body, StrPairs* headers,
        Status::Error status = Status::OK_E
    );

    void wipe() {
        fd = -1;

        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;

        hand_failures = 0;

        r.clear();

        inbuf.cursor = 0;
        outbuf.cursor = 0;
    }
};
