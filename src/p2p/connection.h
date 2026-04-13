#pragma once
#include "p2p/pkt.h"
#include "msg.h"
#include "utils/lru.h"
#include <sys/epoll.h>
#include <vector>

namespace p2p {
class Manager;
}

namespace conn {
enum Status {
    New,
    CryptoSyn,
    CryptoSynAck,
    CryptoAck,
    Live,
    Dead,
    Failed,
};

class Connection {
public:
    uint8_t     version_;
    int         fd_;
    ConnID      id_;
    ConnNode*   lru_node_;
    uint32_t        events_;
    uint8_t         failure_count_;
    conn::Status    status_;


    Key         remote_auth_key_;
    Key         remote_session_key_;
    Key         rx_key_;
    Key         tx_key_;
    KeyPair     session_keys_;


    std::vector<Packet*>    wpkts_;
    Packet                  rpkt_;

    msg::Error marshal_n_enqueue_msg(
        Packet* pkt, const Key& key, uint16_t code,
        std::byte* data, size_t len
    );

    Packet* write_();
    msg::Error read_(p2p::Manager& netman);

    inline bool is_epollout_enabled() { 
        return (events_ & EPOLLOUT) != 0; 
    }
    inline bool has_data_to_write() {
        return wpkts_.size() > 0;
    }
    bool disable_epollout(int epfd);
    bool enable_epollout(int epfd);

    msg::Error syn(p2p::Manager& man);
    msg::Error syn_ack(p2p::Manager& man);
    msg::Error ack(p2p::Manager& man);

    void clear();
};
}
