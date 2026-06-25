#include "connection.h"
#include "manager.h"
#include <arpa/inet.h>
#include <format>


int dial(const char* ip, uint16_t port) {
    // Try IPv4 first
    sockaddr_in server_addr{};
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) == 1) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return -1;

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);

        if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            return -1;
        }
        return sock;
    }

    // Try IPv6
    sockaddr_in6 server_addr6{};
    if (inet_pton(AF_INET6, ip, &server_addr6.sin6_addr) == 1) {
        int sock = socket(AF_INET6, SOCK_STREAM, 0);
        if (sock < 0) return -1;

        server_addr6.sin6_family = AF_INET6;
        server_addr6.sin6_port = htons(port);

        if (connect(sock, (sockaddr*)&server_addr6, sizeof(server_addr6)) < 0) {
            close(sock);
            return -1;
        }
        return sock;
    }

    // Invalid IP string
    return -1;
}

std::optional<Error> P2P::connect(
    Key& pubkey,
    ConnID* id
) {
    auto it = citizens_.map_.find(pubkey);
    if (it == citizens_.map_.end()) {
        return Error{ .msg = std::format("Not a citizen {}.", key_to_str(pubkey)) };
    }
    Citizen& citizen = it->second;

    if (!citizen.is_trustworthy()) {
        return Error{ .msg = std::format("Not Trustworth Citizen {}.", key_to_str(pubkey)) };
    }

    auto existing = key_to_conn_.find(pubkey);
    if (existing != key_to_conn_.end()) {
        *id = existing->second;
        return std::nullopt;
    }

    int fd = dial(citizen.ip_v4, citizen.port_);
    if (fd == -1) {
        return Error{ .r = -1, .msg = std::format("Failed Dial {}.", key_to_str(pubkey)) };
    }

    *id = 0;
    *id = add_socket(fd, pubkey, false);
    if (*id == 0) {
        return Error{ .msg = std::format("Failed Add Socket {}.", key_to_str(pubkey)) };
    }

    // START NEGOTIATION PROCESS
    Connection& c = connections_[*id];
    Error e = conn::syn(c, *this);
    if (e.is_err()) {
        remove_socket(*id);
        return e;
    }

    return e;
}
