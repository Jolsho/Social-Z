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
                continue;

            } else if (fd == from_main_.get_event_fd()) {
                // INTERNAL MSGS
                int k = 0;
                while (Msg* msg = from_main_.pop()) {
                    if (!msg->is_wiped && msg->code < Code::ERRORS) {

                        Error e = handlers::handle_msg(*this, msg);
                        if (e.is_err()) handlers::handle_error(*this, e);

                    } else if (!msg->is_wiped) {

                        // HANDLE ERROR MSG
                        Error e {
                            .id     = msg->id,
                            .code   = (Code)msg->code,
                        };

                        size_t size_r = sizeof(e.r);
                        if (msg->data_len > size_r) {
                            memcpy(msg->data, &e.r, size_r);
                            e.msg.resize(msg->data_len - size_r);
                            if (e.msg.size() > 0) {
                                e.msg.copy(
                                    (char*)msg->data + size_r, 
                                    msg->data_len - size_r
                                );
                            }
                        }
                        handlers::handle_error(*this, e);
                    }

                    if (!msg->is_wiped) msg_wipe(msg);

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
                continue;

            }

            // OPEN CONNECTIONS WITH WORK TO DO
            ConnID id = sock_ids_[fd];
            conn::Connection& conn = connections_[id];
            int _ = lru_.use(conn.lru_node_);

            static constexpr uint8_t MAX_FAILURE = 12;
            if (conn.failure_count_ > MAX_FAILURE) remove_socket(id);

            if (conn.status_ == conn::Status::Failed || 
                conn.status_ == conn::Status::Dead
            ) continue;

            if (events[i].events & EPOLLIN) {
                Error e = conn.read_(*this);
                if (e.is_err()) {
                    handlers::handle_error(*this, e);
                }
            }
            if (events[i].events & EPOLLOUT) {

                if (Packet* pkt = conn.write_()) {
                    pkts_.push_back(pkt);
                }

                if (!conn.has_data_to_write()) 
                    conn.disable_epollout(epoll_fd_);
            }
            
        }

        // HANDLE EXPIRED CONNECTIONS
        for (auto idx: lru_.remove_expired()) {
            remove_socket(idx);
        }
    }
}

