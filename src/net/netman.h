#pragma once
#include "msging.h"
#include "net.h"
#include <sodium/crypto_secretstream_xchacha20poly1305.h>
#include <unordered_map>

using Key = unsigned char[crypto_secretstream_xchacha20poly1305_KEYBYTES];
struct Connection {
    int         fd;
    Key         key;
    uint32_t    events;

    net::ConnectionStatus   status;
    std::vector<uint8_t>    buffer;
};

namespace netman {
class NetworkManager {
    int epoll_fd_;
    msging::SPSCQueue&  to_main_;
    msging::SPSCQueue&  from_main_;

    uint64_t                next_id_;
    std::vector<uint64_t>   free_ids_;

    std::unordered_map<int, uint64_t>   sock_ids_;
    std::vector<Connection>             connections_;

public:
    NetworkManager(msging::ChannelPair& chan);
    uint64_t add_socket(int sock_fd);
    void poll_loop();

private:
    void handle_read(Connection &conn);
    void handle_write(Connection &conn);
    void handle_control_pkt(net::Packet* pkt);
};
}
