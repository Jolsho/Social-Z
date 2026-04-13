#pragma once
#include "codes.h"
#include <string>
#include <sys/epoll.h>
#include <vector>
#include <sys/eventfd.h>
#include <unistd.h>

using ConnID = uint16_t;

namespace msg {

static constexpr size_t MAX_BUFFER_SIZE = 1024 * 4;

class Msg {

public:
    bool        is_wiped;
    uint8_t     too;
    uint8_t     from;

    ConnID      id;
    int         mid;
    Code        code;

    std::vector<std::byte> data;

    Msg(Actors from, size_t cap = MAX_BUFFER_SIZE) {
        data.reserve(cap);
        from = from;
    }

    void wipe() {
        is_wiped = true;
        too = 0;
        id = 0;
        mid = 0;
        code = Code::CTRL;
        data.resize(0);
    }
};


struct Error {
    int         r       = -1;
    ConnID      id      = 0;
    Code        code    = Code::E_INTERNAL;
    std::string msg;

    inline bool is_err() const {
        return r != 0 || code != Code::SUCCESS;
    }
};
const Error SUCCESS{0, 0, Code::SUCCESS, {}};


void p2p_close_conn(msg::Msg* msg, Actors from, ConnID id);
void p2p_error(msg::Msg* msg, Error e, Actors from);
int p2p_new_conn(
    msg::Msg* msg, 
    std::string ip,
    uint16_t port,
    std::string& key
);
}
