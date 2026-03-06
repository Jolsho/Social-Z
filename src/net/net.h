#pragma once
#include <cstdint>
#include <vector>

namespace net {

enum ConnectionStatus {
    New,
    Negotiating,
    Live,
    Dead,
};

enum Actors {
    Networker,
    Social,
    FileSys,
};

struct Packet {
    Actors      too;
    Actors      from;
    uint64_t    id;
    uint8_t     kind;

    uint32_t        length;
    std::vector<uint8_t> data;
};

void wipe_packet(Packet* pack);

int dial(const char* ip, unsigned short port);

bool is_epollout_enabled(uint32_t events);
uint32_t enable_epollout(int epfd, int fd, uint32_t current_events);
uint32_t disable_epollout(int epfd, int fd, uint32_t current_events);

}

