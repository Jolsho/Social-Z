#include <arpa/inet.h>
#include <cstdio>
#include <fcntl.h>
#include <sys/epoll.h>
#include "api/actor.h"
#include "manager.h"
#include "utils/shutdown.h"

void P2P::poll_loop() {
    const int MAX_EVENTS = 64;
    EventBuffer* events = new_event_buffer(MAX_EVENTS);


    while (true) {
        poll_actor(chans_, events, in_msgs_, free_out_msgs_, 500);
        if (ChanStatsPair* stats = poll_telemetry(chans_)) logr_->log(stats);

        if (events->size < 0) {
            logr_->log("p2p:poll returned error.", events->size);
            should_shutdown = true;
        }

        // INTERNAL MSGS
        while (Msg* msg = consume_msg(in_msgs_)) {
            Error e = handle_msg(msg);
            if (e.is_err()) handle_error(e);
        }

        // ALL OTHER EVENTS
        for (int i = 0; i < events->size; i++) {
            EpollEvent& ev = events->events[i];

            if (ev.data.fd == listen_fd_) {
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
            }

            // OPEN CONNECTIONS WITH WORK TO DO
            ConnID id = ev.data.u64;
            if (id > connections_.size()) continue;

            Connection& conn = connections_[id];
            int _ = lru_.use(conn.lru_node_);

            static constexpr uint8_t MAX_FAILURE = 12;
            if (conn.failure_count_ > MAX_FAILURE) remove_socket(id);

            if (conn.status_ == conn::Status::Failed || 
                conn.status_ == conn::Status::Dead
            ) continue;

            if (ev.events & EPOLLIN) {
                Error e = readable_conn(conn);
                if (e.is_err()) handle_error(e);
            }
            if (ev.events & EPOLLOUT) {
                Error e = writeable_conn(conn);
                if (e.is_err()) handle_error(e);
            }
        }

        // HANDLE EXPIRED CONNECTIONS
        time_t now = time(nullptr);
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

        update_actor(chans_, &in_msgs_->consumed_, &free_out_msgs_->consumed_);

        if (should_shutdown) {
            shutdown();
            return;
        }

    }
}

