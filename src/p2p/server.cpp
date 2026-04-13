#include <arpa/inet.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <format>
#include <sodium/crypto_box.h>
#include <sodium/crypto_kx.h>
#include "p2p/p2p.h"
#include "utils/lru.h"

ConnID p2p::Manager::add_socket(int sock_fd, const Key pubkey, bool is_inbound) {

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
    conn::Connection& c = connections_[id];

    // Store connection
    c.fd_               = sock_fd;
    c.id_               = id;
    c.lru_node_->key    = id;
    c.session_keys_     = session;
    c.events_           = ev.events;
    c.status_           = (is_inbound) ? 
                            conn::Status::CryptoSynAck : 
                            conn::Status::CryptoSyn;
    c.rpkt_.cursor_     = 0;

    int evicted = lru_.use(c.lru_node_);
    if (evicted > 0) {
        remove_socket(evicted);
    }

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
    }

    connections_[id] = c;

    return id;
}


void p2p::Manager::remove_socket(ConnID id) {
    conn::Connection &conn = connections_[id];
    if (conn.status_ == conn::Status::Dead) return;

    conn.status_ = conn::Status::Dead;
    lru_.remove(conn.lru_node_);
    close(conn.fd_);
    conn.clear();
    sock_ids_.erase(conn.fd_);
    free_ids_.push_back(id);
}


int p2p::Manager::start_server(P2PConfig& conf) {

    // TODO -- load in citizen list
    // citizens.reserve(count + some_buffer_count);

    if (memcmp(conf.key.data(), ZERO_KEY.data(), KEY_SIZE) == 0) {
        int key_fd = open(conf.key_path, O_RDWR | O_CREAT, 0600);
        if (key_fd < 0) {
            logr_->log(std::format("OPEN KEY_FILE FAILED %d", key_fd));
            return -1;
        }

        lseek(key_fd, 0, SEEK_SET);
        if (read(key_fd, keys_.priv.data(), KEY_SIZE) < KEY_SIZE) {

            int r = crypto_box_keypair(keys_.pub.data(), keys_.priv.data());
            if (r < 0) {
                logr_->log(std::format("GENERATING KEYS FAILED %d", r));
                close(key_fd);
                remove(conf.key_path);
                return -1;
            }

            lseek(key_fd, 0, SEEK_SET);
            if (write(key_fd, keys_.priv.data(), KEY_SIZE) < KEY_SIZE) {
                close(key_fd);
                remove(conf.key_path);
                logr_->log("PERSISTING KEYS FAILED");
                return -1;
            }
        }
    } else {
        memcpy(keys_.priv.data(), conf.key.data(), KEY_SIZE);
    }


    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        logr_->log(std::format("EPOLL_CREATE1 FAILED %d", epoll_fd_));
        return -1;
    }

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    int r = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (r < 0) {
        logr_->log(std::format("SOCKET OPT FAILED: %d", r));
        return r;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(conf.port);
    if (conf.ip == nullptr) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        r = inet_pton(AF_INET, conf.ip, &addr.sin_addr);
        if (r < 0) {
            close(listen_fd_);
            logr_->log(std::format("INVALID IP: %d", r));
            return r;
        }
    }

    r = bind(listen_fd_, (sockaddr*)&addr, sizeof(addr));
    if (r < 0) {
        logr_->log(std::format("BIND FAILED: %d", r));
        return r;
    }

    r = listen(listen_fd_, SOMAXCONN);
    if (r < 0) {
        logr_->log(std::format("LISTEN FAILED: %d", r));
        return r;
    }
    
    int flags = fcntl(listen_fd_, F_GETFL, 0);
    if (flags < 0) {
        logr_->log(std::format("FCNTL FAILED: %d", r));
        return r;
    }

    r = fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK);
    if (r < 0) {
        logr_->log(std::format("SET FLAGS FAILED: %d", r));
        return r;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd_;

    r = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);
    if (r < 0) {
        logr_->log(std::format("EPOLL ADDING FAILED: %d", r));
        return r;
    }
    return 0;
}
