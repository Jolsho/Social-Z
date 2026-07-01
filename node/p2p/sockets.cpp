#include "sz/api/actor.h"
#include "connection.h"
#include "manager.h"
#include "sodium/crypto_kx.h"
#include "sz/crypto.h"
#include "sz/utils/error.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>


ConnID P2P::add_socket(int sock_fd, const Key pubkey, bool is_inbound) {

    // Make non-blocking
    int flags = fcntl(sock_fd, F_GETFL, 0);
    if (flags == -1) return 0;

    if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) == -1) return 0;

    // Add to epoll
    EpollEvent ev{};

    ev.events = EPOLLIN | EPOLLET;
    if (!is_inbound) ev.events |= EPOLLOUT;

    // Allocate ID
    ConnID id;
    next_id(&id);
    
    ev.data.fd = sock_fd;
    ev.data.u64 = id;
    if (ctl_epoll(chans_, &ev, EPOLL_CTL_ADD) < 0) {
        close(sock_fd);
        put_back_id(id);
        return 0;
    }

    KeyPair session{};
    crypto_kx_keypair(session.pub.b, session.priv.b);
    Connection& c = connections_[id];

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

    if (key_is_zero(&pubkey)) {
        memcpy(c.keys_.remote_auth_.b, pubkey.b, KEY_SIZE);

        // DERIVE INITIAL SHARED SECRET WITH AUTH KEYS
        int r = crypto_kx_client_session_keys(
            c.keys_.rx_.b, c.keys_.tx_.b, 
            keys_.pub.b, 
            keys_.priv.b, 
            c.keys_.remote_auth_.b
        );
        if (r != 0) return 0;
    }

    connections_[id] = c;
    negotiating_timeouts_.push_back({id, time(nullptr) + NEGOTIATION_TIMEOUT});

    return id;
}


void P2P::remove_socket(ConnID id) {
    Connection &conn = connections_[id];
    if (conn.status_ == conn::Status::Dead) return;

    conn.status_ = conn::Status::Dead;
    lru_.remove(conn.lru_node_);
    close(conn.fd_);
    conn::clear(conn);
    sock_ids_.erase(conn.fd_);
    put_back_id(id);
    key_to_conn_.erase(conn.keys_.remote_auth_);

    buffers_.put(conn.rpkt_.buff_);
    buffers_.put(conn.wpkt_.buff_);

    for (const int mid: conn.pending_ids_) {
        messenger_.next(mid);
    }
}

int dial(const char* ip, uint16_t port) {
    // Try IPv4 first
    sockaddr_in server_addr{};
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) == 1) {
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
    if (inet_pton(AF_INET6, ip, &server_addr6.sin6_addr) == 1) {
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

std::optional<Error> P2P::connect(
    Key& pubkey,
    ConnID* id
) {
    auto it = citizens_.map_.find(pubkey);
    if (it == citizens_.map_.end()) {
        return Error{ 
            .r = -1,
            .key = pubkey,
            .msg = "connect() :: Not a citizen"
        };
    }
    Citizen& citizen = it->second;

    if (!citizen.is_trustworthy()) {
        return Error{ 
            .r = -1,
            .key = pubkey,
            .msg = "connect() :: Not Trustworthy Citizen"
        };
    }

    auto existing = key_to_conn_.find(pubkey);
    if (existing != key_to_conn_.end()) {
        *id = existing->second;
        return std::nullopt;
    }

    int fd = dial(citizen.ip_v4, citizen.port_);
    if (fd == -1) {
        return Error{ 
            .r = -1,
            .code = E_INTERNAL,
            .key = pubkey,
            .msg = "connect() :: Failed Dial"
        };
    }

    *id = 0;
    *id = add_socket(fd, pubkey, false);
    if (*id == 0) {
        return Error{ 
            .r = -1,
            .code = E_INTERNAL,
            .key = pubkey,
            .msg = "connect() :: Failed Add Socket"
        };
    }

    // START NEGOTIATION PROCESS
    Connection& c = connections_[*id];
    Error e = conn::syn(c, *this);
    if (e.is_err()) {
        remove_socket(*id);
        return e;
    }

    return e;
}

