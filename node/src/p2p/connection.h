#pragma once
#include "utils/error.h"
#include "p2p/pkt.h"
#include "utils/lru.h"
#include <cstdint>
#include <deque>
#include <sys/epoll.h>

namespace p2p {
class Manager;
}

namespace conn {
enum class Status : uint8_t {
    CryptoSyn,
    CryptoSynAck,
    CryptoAck,
    Live,
    Dead,
    Failed,
};
struct ConnKeys {
    Key         remote_auth_;
    Key         remote_session_;
    Key         rx_;
    Key         tx_;
    KeyPair     session_;
};

class Connection {
public:
    static constexpr size_t MAX_PENDING_OUT = 32;

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

    int write_(p2p::Manager& netman);
    Error read_(BufferStore& buffs);

    inline bool is_epollout_enabled() { 
        return (events_ & EPOLLOUT) != 0; 
    }
    bool disable_epollout(int epfd);
    bool enable_epollout(int epfd);

    Error syn(p2p::Manager& man);
    Error syn_ack(p2p::Manager& man);
    Error ack(p2p::Manager& man);

    void clear();
};
}
