#include <arpa/inet.h>
#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <sodium/crypto_kx.h>
#include "net/net.h"

ConnID net::Manager::add_socket(int sock_fd, const Key pubkey, bool is_inbound) {

    // Make non-blocking
    int flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags == -1) return 0;

    if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) == -1) return 0;

    // Add to epoll
    epoll_event ev{};

    ev.events = EPOLLIN | EPOLLET;
    if (!is_inbound) ev.events |= EPOLLOUT;
    
    ev.data.fd = sock_fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sock_fd, &ev) < 0) {
        close(sock_fd);
        return 0;
    }

    // Allocate ID
    ConnID id;
    if (!free_ids_.empty()) {
        id = free_ids_.back();
        free_ids_.pop_back();
    } else {
        id = next_id_++;
    }

    KeyPair session{};
    crypto_kx_keypair(session.pub.data(), session.priv.data());

    // Store connection
    conn::Connection c = {
        .fd_        = sock_fd,
        .id_        = id,
        .session_keys_ = session,
        .events_    = ev.events,
        .status_    = (is_inbound) ? 
                        conn::Status::CryptoSynAck : 
                        conn::Status::CryptoSyn,
        .rpkt_      = {}
    };

    expirations_[id] = std::time(nullptr);
    if (pubkey != ZERO_KEY) {
        memcpy(c.remote_auth_key_.data(), pubkey.data(), pubkey.size());

        // DERIVE INITIAL SHARED SECRET WITH AUTH KEYS
        int r = crypto_kx_client_session_keys(
            c.rx_key_.data(), c.tx_key_.data(), 
            keys_.pub.data(), 
            keys_.priv.data(), 
            c.remote_auth_key_.data()
        );
        if (r != 0) return 0;

        expirations_[id] += CONNECTION_TIMEOUT;

    } else {
        expirations_[id] += REVEAL_KEY_TIMEOUT;
    }

    connections_[id] = c;

    return id;
}


void net::Manager::remove_socket(ConnID id) {
    conn::Connection &conn = connections_[id];
    close(conn.fd_);
    conn.clear();
    sock_ids_.erase(conn.fd_);
    free_ids_.push_back(id);
}


int net::Manager::start_server(const char* ip, ConnID port) {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        perror("epoll_create1");
        return -1;
    }

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    int r = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (r <= 0) {
        printf("BAD SOCKET OPT :: %d\n", r);
        return r;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (ip == nullptr) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        r = inet_pton(AF_INET, ip, &addr.sin_addr);
        if (r <= 0) {
            close(listen_fd_);
            printf("NOT VALID IP :: %d\n", r);
            return r;
        }
    }

    r = bind(listen_fd_, (sockaddr*)&addr, sizeof(addr));
    if (r <= 0) {
        printf("FAILED BIND :: %d\n", r);
        return r;
    }

    r = listen(listen_fd_, SOMAXCONN);
    if (r <= 0) {
        printf("FAILED LISTEN :: %d\n", r);
        return r;
    }
    
    int flags = fcntl(listen_fd_, F_GETFL, 0);
    r = fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK);
    if (r <= 0) {
        printf("FAILED FLAG SETTING :: %d\n", r);
        return r;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd_;

    r = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);
    if (r <= 0) {
        printf("FAILED ADDING LIST TO EPOLL :: %d\n", r);
        return r;
    }
    return 0;
}
