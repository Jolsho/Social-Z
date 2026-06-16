#include <arpa/inet.h>
#include <cstdio>
#include <fcntl.h>
#include "p2p/connection.h"
#include "p2p/p2p.h"
#include "utils/chans.h"
#include "utils/shutdown.h"

void p2p::Manager::poll_loop() {
    const int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];

    while (true) {
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 0); // short timeout
        time_t now = time(nullptr);
        if (should_shutdown) {
            shutdown();
            return;
        }
        auto stats = chans_.poll_telemetry();
        if (stats) {
            logr_->log(stats);
        }

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
                    if (id == 0) close(client);
                }
                continue;

            } else if (fd == chans_.get_event_fd()) {
                // INTERNAL MSGS
                Msg* msg;
                while (chans_.in_.front(msg)) {
                    Error e = handle_msg(msg);
                    if (e.is_err()) handle_error(e);
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
                Error e = readable_conn(conn);
                if (e.is_err()) {
                    handle_error(e);
                }
            }
            if (events[i].events & EPOLLOUT) {
                Error e = writeable_conn(conn);
                if (e.is_err()) {
                    handle_error(e);
                }
            }
        }


        // HANDLE EXPIRED CONNECTIONS
        while (negotiating_timeouts_.size() > 0) {
            auto [id, timeout] = negotiating_timeouts_.front();
            if (timeout > now ) break;

            if (connections_[id].status_ != conn::Status::Live)
                remove_socket(id);

            negotiating_timeouts_.pop_front();
        }

        for (auto idx: lru_.remove_expired()) {
            remove_socket(idx);
        }
    }
}

