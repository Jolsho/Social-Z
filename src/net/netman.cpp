#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sodium/crypto_aead_chacha20poly1305.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include "netman.h"
#include "net/net.h"

netman::NetworkManager::NetworkManager(msging::ChannelPair& chan)
    : from_main_(chan.to), to_main_(chan.from) 
{
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) perror("epoll_create1");
}

uint64_t netman::NetworkManager::add_socket(int sock_fd) {
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET; // edge-triggered
    ev.data.fd = sock_fd;
    fcntl(sock_fd, F_SETFL, fcntl(sock_fd, F_GETFL) | O_NONBLOCK);
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sock_fd, &ev);

    uint64_t id;
    if (free_ids_.size() > 0) {
        id = free_ids_.back();
        free_ids_.pop_back();
    } else {
        id = next_id_++;
    }
    connections_[id] = {
        .fd     = sock_fd,
        .events = ev.events,
        .status = net::ConnectionStatus::New,
        .buffer = {},
    };

    return id;
}

const uint8_t NET_CONTROL_PKT = 1;

void netman::NetworkManager::poll_loop() {
    const int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];

    while (true) {
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1); // short timeout
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            auto it = sock_ids_.find(fd);
            if (it != sock_ids_.end()) {
                uint32_t ev = events[i].events;
                if (ev & EPOLLIN) handle_read(connections_[it->second]);
                if (ev & EPOLLOUT) handle_write(connections_[it->second]);
            }

            int k = 0;
            while (net::Packet* pkt = from_main_.pop()) {
                if (pkt->kind == NET_CONTROL_PKT) {
                    handle_control_pkt(pkt);
                    continue;
                }

                Connection& conn = connections_[pkt->id];
                if (conn.status != net::ConnectionStatus::Dead) {
                    if (!net::is_epollout_enabled(conn.events)) {
                        conn.events = net::enable_epollout(epoll_fd_, conn.fd, conn.events);
                    }
                    sock_pkt_queue_[id].push(pkt);
                } else {
                    net::wipe_packet(pkt);
                    to_main_.push(pkt);
                }

                if (k++ > 32) break;
            }
        }
    }
}

void netman::NetworkManager::handle_read(Connection& conn) {
    uint8_t buffer[4096];
    while (true) {
        ssize_t bytes = read(conn.fd, buffer, sizeof(buffer));
        if (bytes <= 0) break;

        conn.buffer.insert(conn.buffer.end(), buffer, buffer + bytes);

        // Extract packets
        while (conn.buffer.size() >= sizeof(uint32_t)) {
            uint32_t pkt_len;
            memcpy(&pkt_len, conn.buffer.data(), sizeof(uint32_t));

            if (conn.buffer.size() < pkt_len + sizeof(uint32_t)) break;

            net::Packet* pkt = new net::Packet;
            pkt->length = pkt_len;
            pkt ->data.insert(pkt->data.end(), conn.buffer.begin() + 4, conn.buffer.begin() + 4 + pkt_len);
            to_main_.push(std::move(pkt));

            conn.buffer.erase(conn.buffer.begin(), conn.buffer.begin() + 4 + pkt_len);
        }
    }
}

void netman::NetworkManager::handle_write(Connection& conn) {
    while (auto pkt = from_main_.pop()) {
        // Prepend length
        uint32_t len = pkt->length;
        std::vector<uint8_t> out_buf(sizeof(uint32_t) + pkt->data.size());
        memcpy(out_buf.data(), &len, sizeof(uint32_t));
        memcpy(out_buf.data() + sizeof(uint32_t), pkt->data.data(), pkt->data.size());

        ssize_t written = write(conn.fd, out_buf.data(), out_buf.size());
        if (written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // Could not write, push back and try later
            to_main_.push(std::move(pkt));
            break;
        }
    }
}

void netman::NetworkManager::handle_control_pkt(net::Packet* pkt) {
    // TODO -- handle a new key or close connection or something like that
    //
}

