#include <arpa/inet.h>
#include <cstdio>
#include <fcntl.h>
#include "msg.h"
#include "p2p/p2p.h"
#include "p2p/handlers.h"

void p2p::Manager::poll_loop() {
    const int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];

    while (true) {
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 0); // short timeout
        time_t now = time(nullptr);

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == listen_fd_) {
                // NEW CONNECTION
                while (true) {
                    sockaddr_storage client_addr{};
                    socklen_t addr_len = sizeof(client_addr);

                    int client = accept(listen_fd_, (sockaddr*)&client_addr, &addr_len);

                    if (client == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        break;
                    }

                    ConnID id = add_socket(client, ZERO_KEY, true);
                    if (id == 0) {
                        close(client);
                        continue;
                    }
                }

            } else if (fd == from_main_.get_event_fd()) {
                // INTERNAL MSGS
                int k = 0;
                while (msg::Msg* msg = from_main_.pop()) {
                    if (!msg->is_wiped && msg->code < CODE::ERRORS) {
                        handlers::handle_msg(*this, msg);

                    } else if (!msg->is_wiped) {

                        // HANDLE ERROR MSG
                        net_msg::Error e {
                            .id     = msg->id,
                            .code   = (CODE)msg->code,
                        };

                        size_t size_r = sizeof(e.r);
                        if (msg->data.size() > size_r) {
                            memcpy(msg->data.data(), &e.r, size_r);
                            e.msg.resize(msg->data.size() - size_r);
                            if (e.msg.size() > 0) {
                                e.msg.copy(
                                    (char*)msg->data.data() + size_r, 
                                    msg->data.size() - size_r
                                );
                            }
                        }
                        handlers::handle_error(*this, e);
                    }

                    if (!msg->is_wiped) msg->wipe();

                    if (msg->from == Actors::PEERNET) {
                        if (msgs_.size() < msgs_.capacity()) {
                            msgs_.push_back(msg);
                        } else {
                            delete msg;
                        }
                    } else {
                        if (!to_main_.push(msg)) {
                            delete msg;
                        }
                    }

                    if (++k > 32) break;
                }

            } else {

                // OPEN CONNECTIONS WITH WORK TO DO
                auto it = sock_ids_.find(fd);
                if (it != sock_ids_.end()) {

                    uint32_t ev = events[i].events;
                    ConnID id = it->second;

                    conn::Connection& conn = connections_[id];
                    expirations_[id] = now + CONNECTION_TIMEOUT;

                    if (conn.status_ == conn::Status::Failed || 
                        conn.status_ == conn::Status::Dead
                    ) continue;

                    if (ev & EPOLLIN) {
                        net_msg::Error e = conn.read_(*this);
                        if (e.is_err()) {
                            handlers::handle_error(*this, e);
                        }
                    }
                    if (ev & EPOLLOUT) {
                        if (Packet* pkt = conn.write_()) {
                            pkts_.push_back(pkt);
                        }
                        if (!conn.has_data_to_write()) 
                            conn.disable_epollout(epoll_fd_);
                    }
                }
            }
        }
        
        // HANDLE EXPIRED CONNECTIONS
        for (int i{ 0 }; i < expirations_.size(); i++) {
            if (expirations_[i] < now) remove_socket(i);
        }

    }
}

