#include <arpa/inet.h>
#include <format>
#include "net/handlers.h"

int dial(const std::string& ip, uint16_t port) {
    // Try IPv4 first
    sockaddr_in server_addr{};
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) == 1) {
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
    if (inet_pton(AF_INET6, ip.c_str(), &server_addr6.sin6_addr) == 1) {
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

void handlers::handle_error(net::Manager& man, net_msg::Error e) {
    conn::Connection& conn = man.connections_[e.id];

    if (conn.remote_auth_key_ != ZERO_KEY) {
        Citizen& citizen = man.citizens_.at(conn.remote_auth_key_);
        citizen.record_infringement(e.code);

        if (!citizen.is_trustworthy())
            man.remove_socket(e.id);
    }
}

void handlers::handle_msg(net::Manager& man, msg::Msg* msg) {
    switch (msg->code) {
        case CODE::WRITE: {
            conn::Connection &conn = man.connections_[msg->id];
            if (!conn.is_epollout_enabled()) {
                conn.events_ = conn.enable_epollout(man.epoll_fd_);
            }

            net_msg::Error e = conn.marshal_n_enqueue_msg(man.get_pkt(), man.keys_.pub, msg->code, msg->data.data(), msg->data.size());
            if (e.is_err()) {
                handle_error(man, e);
            }
            break;
        }

        case CODE::CLOSE_CONN: {
            conn::Connection &conn = man.connections_[msg->id];

            if (conn.status_ == conn::Status::Live && msg->data.size() > 0) {
                net_msg::Error e = conn.marshal_n_enqueue_msg(man.get_pkt(), man.keys_.pub, msg->code, msg->data.data(), msg->data.size());
                if (e.is_err()) {
                    handle_error(man, e);
                }

                // Force write now
                if (Packet* pack = conn.write_()) {
                    man.pkts_.push_back(pack);
                }

                while (conn.wpkts_.size() > 0) {
                    man.pkts_.push_back(conn.wpkts_.back());
                    conn.wpkts_.pop_back();
                }
            }

            man.remove_socket(msg->id);
            break;
        }

        case CODE::NEW_CONN: {
            std::byte* cursor = msg->data.data();

            std::string ip{(char*)cursor};
            cursor += ip.size();

            uint16_t port = read_from_cursor<uint16_t>(cursor);

            Key pubkey;
            cursor += str_to_key(reinterpret_cast<const char*>(cursor), pubkey);
            

            auto it = man.citizens_.find(pubkey);
            if (it == man.citizens_.end()) {
                handle_error(man, { .msg = std::format("Not a citizen {}.", key_to_str(pubkey)) });
                return;
            }
            Citizen& citizen = it->second;

            if (!citizen.is_trustworthy()) {
                handle_error(man, { .msg = std::format("Not Trustworth Citizen {}.", key_to_str(pubkey)) });
                return;
            }

            int fd = dial(ip, port);
            if (fd == -1) {
                handle_error(man, { .msg = std::format("Failed Dial {}.", key_to_str(pubkey)) });
                return;
            }

            ConnID id = man.add_socket(fd, pubkey, false);
            if (id == 0) {
                handle_error(man, { .msg = std::format("Failed Add Socket {}.", key_to_str(pubkey)) });
                return;
            }

            // START NEGOTIATION PROCESS
            net_msg::Error e = man.connections_[id].syn(man);
            if (e.is_err()) {
                handle_error(man, e);
            }

            break;
        }

        case CODE::SHUTDOWN: {
            // TODO -- shutdown server
        }

        default: return;
    }
}

