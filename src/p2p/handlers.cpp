#include <arpa/inet.h>
#include <format>
#include "p2p/handlers.h"

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

void handlers::handle_error(p2p::Manager& man, Error e) {
    conn::Connection& conn = man.connections_[e.id];

    if (conn.remote_auth_key_ != ZERO_KEY) {
        Citizen& citizen = man.citizens_.at(conn.remote_auth_key_);
        citizen.record_infringement(e.code);

        if (!citizen.is_trustworthy())
            man.remove_socket(e.id);
    }

    man.logr_->log(e.msg);
}

Error handlers::handle_msg(p2p::Manager& man, Msg* msg) {
    switch (msg->code) {

        case Code::CLOSE_CONN: {
            conn::Connection &conn = man.connections_[msg->id];

            if (conn.status_ == conn::Status::Live && msg->data_len > 0) {
                Error e = conn.marshal_n_enqueue_msg(man.get_pkt(), man.keys_.pub, 
                    msg->code, 
                    reinterpret_cast<std::byte*>(msg->data), 
                    msg->data_len
                );
                if (e.is_err()) return e;

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

        case Code::NEW_CONN: {
            std::byte* cursor = reinterpret_cast<std::byte*>(msg->data);

            std::string ip{(char*)cursor};
            cursor += ip.size();

            uint16_t port;
            memcpy(&port, cursor, sizeof(uint16_t));

            Key pubkey;
            memcpy(pubkey.data(), cursor, KEY_SIZE);
            cursor += KEY_SIZE;
            

            auto it = man.citizens_.find(pubkey);
            if (it == man.citizens_.end()) {
                return { .msg = std::format("Not a citizen {}.", key_to_str(pubkey)) };
            }
            Citizen& citizen = it->second;

            if (!citizen.is_trustworthy()) {
                return { .msg = std::format("Not Trustworth Citizen {}.", key_to_str(pubkey)) };
            }

            int fd = dial(ip, port);
            if (fd == -1) {
                return { .msg = std::format("Failed Dial {}.", key_to_str(pubkey)) };
            }

            ConnID id = man.add_socket(fd, pubkey, false);
            if (id == 0) {
                return { .msg = std::format("Failed Add Socket {}.", key_to_str(pubkey)) };
            }

            // START NEGOTIATION PROCESS
            Error e = man.connections_[id].syn(man);
            if (e.is_err()) return e;

            break;
        }

        case Code::SHUTDOWN: {
            // TODO -- shutdown server
            break;
        }

        case Code::PING: {
            // TODO -- parse key find conn.
            // if not exists just say fuck it and quit
            break;
        }

        default: {
            conn::Connection &conn = man.connections_[msg->id];
            if (conn.status_ == conn::Status::Live) {
                if (!conn.is_epollout_enabled()) {
                    conn.events_ = conn.enable_epollout(man.epoll_fd_);
                }

                Error e = conn.marshal_n_enqueue_msg(
                    man.get_pkt(), man.keys_.pub, msg->code, 
                    reinterpret_cast<std::byte*>(msg->data), 
                    msg->data_len
                );
                if (e.is_err()) return e;
            }
            break;
        }
    }
    return ESUCCESS;
}

