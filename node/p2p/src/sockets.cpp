#include "p2p.h"
#include "sodium/crypto_kx.h"


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
    next_id(&id);

    KeyPair session{};
    crypto_kx_keypair(session.pub.data(), session.priv.data());
    conn::Connection& c = connections_[id];

    // Store connection
    c.fd_               = sock_fd;
    c.id_               = id;
    c.lru_node_->key    = id;
    c.keys_.session_    = session;
    c.events_           = ev.events;
    c.status_           = (is_inbound) ? 
                            conn::Status::CryptoSynAck : 
                            conn::Status::CryptoSyn;
    c.rpkt_.wipe();
    key_to_conn_[pubkey] = id;
    
    int evicted = lru_.use(c.lru_node_);
    if (evicted > 0) {
        remove_socket(evicted);
    }

    if (pubkey != ZERO_KEY) {
        memcpy(c.keys_.remote_auth_.data(), pubkey.data(), pubkey.size());

        // DERIVE INITIAL SHARED SECRET WITH AUTH KEYS
        int r = crypto_kx_client_session_keys(
            c.keys_.rx_.data(), c.keys_.tx_.data(), 
            keys_.pub.data(), 
            keys_.priv.data(), 
            c.keys_.remote_auth_.data()
        );
        if (r != 0) return 0;
    }

    connections_[id] = c;
    negotiating_timeouts_.push_back({id, time(nullptr) + NEGOTIATION_TIMEOUT});

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
    put_back_id(id);
    key_to_conn_.erase(conn.keys_.remote_auth_);

    buffers_.put(conn.rpkt_.buff_);
    buffers_.put(conn.wpkt_.buff_);

    for (const int mid: conn.pending_ids_) {
        messenger_.next(mid);
    }
}


