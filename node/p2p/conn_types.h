#pragma once
#include "sz/codec.h"
#include "pkt.h"
#include "sz/utils/lru.hpp"
#include <deque>

namespace  conn {
enum class Status : uint8_t {
    CryptoSyn,
    CryptoSynAck,
    CryptoAck,
    Live,
    Dead,
    Failed,
};
}

struct ConnKeys {
    Key         remote_auth_;
    Key         remote_session_;
    Key         rx_;
    Key         tx_;
    KeyPair     session_;
};

struct Connection {
    int         fd_;
    uint32_t    events_;

    ConnID          id_;

    uint8_t         version_;
    conn::Status    status_;
    uint8_t         failure_count_;

    ConnKeys        keys_;

    std::deque<int> pending_ids_;
    Packet      wpkt_;
    Packet      rpkt_;

    ConnNode*   lru_node_;
};
