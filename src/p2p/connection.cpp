#include <format>
#include "p2p/p2p.h"
#include "p2p/connection.h"

void conn::Connection::clear() {
    fd_ = -1;
    id_ = 0;
    size_t key_s = rx_key_.size();
    memset(&remote_auth_key_, 0, key_s);
    memset(&remote_session_key_, 0, key_s);
    memset(&session_keys_, 0, sizeof(KeyPair));
    memset(&rx_key_, 0, key_s);
    memset(&tx_key_, 0, key_s);
    events_ = 0;
    failure_count_ = 0;
    status_ = Status::Dead;

    memset(rpkt_.body(), 0, rpkt_.cap());
}

bool conn::Connection::enable_epollout(int epfd) {
    struct epoll_event ev{};
    ev.data.fd = fd_;
    ev.events = events_ |= EPOLLOUT;
    return epoll_ctl(epfd, EPOLL_CTL_MOD, fd_, &ev) == -1;
}

bool conn::Connection::disable_epollout(int epfd) {
    struct epoll_event ev{};
    ev.data.fd = fd_;
    ev.events = events_ &= ~EPOLLOUT;
    return epoll_ctl(epfd, EPOLL_CTL_MOD, fd_, &ev) == -1;
}

net_msg::Error conn::Connection::read_(
    p2p::Manager& man
) {
    while (true) {

        // READING PREFIX
        if (rpkt_.cursor_ < Packet::PREFIX_LEN) {
            size_t n = read(fd_, rpkt_.get_cursor(), Packet::PREFIX_LEN);
            if (n <= 0) break;
            rpkt_.cursor_ += n;

            if (rpkt_.cursor_ == Packet::PREFIX_LEN) {
                if (rpkt_.get_body_len() > Packet::BODY_SZ) {
                    rpkt_.wipe();
                    status_ = conn::Status::Failed;
                    return { -1, id_, 
                        CODE::E_OVERSIZED, 
                        std::format("read_() :: MAX Pkt size :: {}",
                            key_to_str(remote_auth_key_)
                        )
                    };
                }
            } else {
                break;
            }
        }

        // READING HEADER
        if (rpkt_.cursor_ < Packet::PREFIX_LEN + Packet::HEADER_LEN) {
            size_t n = read(fd_, rpkt_.get_cursor(), Packet::HEADER_LEN);
            if (n <= 0) break;
            rpkt_.cursor_ += n;

            if (rpkt_.cursor_ < Packet::PREFIX_LEN + Packet::HEADER_LEN) {
                break;
            }
        }

        // READING BODY
        uint64_t target = rpkt_.expected_length();
        if (rpkt_.cursor_ > Packet::PREFIX_LEN + Packet::HEADER_LEN) {
            ssize_t n = read(fd_, rpkt_.get_cursor(), target - rpkt_.cursor_);
            if (n <= 0) break;
            rpkt_.cursor_ += n;
            if (rpkt_.cursor_ < target) break;
        }

        // DECRYPT BODY
        int r = rpkt_.decrypt_body(rx_key_);
        if (r != 0) {
            rpkt_.wipe();
            failure_count_++;

            return { r, id_, 
                CODE::E_MALFORMED, 
                std::format("read_() :: decrypt :: {}", 
                    key_to_str(remote_auth_key_)
                )
            };
        }

        if (status_ == conn::Status::Live) {
            msg::Msg* msg = man.get_msg();

            // MAKE SURE ITS NOT A (CTRL || INTERNAL) CODE
            msg->code = (CODE)rpkt_.get_code();
            msg->too = code_too_too(msg->code);

            if (msg->too == Actors::NONE) {
                msg->wipe();
                man.msgs_.push_back(msg);
                return { 
                    -1, id_, 
                    CODE::E_UNAUTHORIZED, 
                    std::format("read_() :: bad code :: {} :: {}", 
                         (uint16_t)msg->code , key_to_str(remote_auth_key_)
                    )
                };
            }

            msg->from = Actors::PEERNET;
            msg->id = id_;
            msg->is_wiped = false;
            msg->data.insert(msg->data.end(), rpkt_.body(), rpkt_.get_cursor());
            if (!man.to_main_.push(msg)) {
                // TODO 
            }
            rpkt_.wipe();

        } else if (status_ == conn::Status::CryptoSynAck) {
            syn_ack(man);
        } else if (status_ == conn::Status::CryptoAck) {
            ack(man);
        }
    }

    return net_msg::SUCCESS;
}

Packet* conn::Connection::write_() {
    Packet* pkt = wpkts_.front();
    uint64_t total = pkt->expected_length();

    uint64_t written = write(fd_, pkt->get_cursor(), total - pkt->cursor_);
    if (written < 0 && (errno = EAGAIN || errno == EWOULDBLOCK)) {
        return NULL;
    } else if (written < 0) {
        failure_count_++;
    }
    pkt->cursor_ += written;

    if (pkt->cursor_ == total) {
        return *wpkts_.erase(wpkts_.begin()).base();
    }
    return nullptr;
}

net_msg::Error conn::Connection::marshal_n_enqueue_msg(
    Packet* pkt, 
    const Key& key,
    uint16_t code,
    std::byte* data,
    size_t len
) {
    if (wpkts_.size() >= wpkts_.capacity()) 
        return {-1, id_, CODE::E_INTERNAL, "Write Buffer CAP reached."};

    pkt->set_len(len);
    pkt->set_version(version_);
    pkt->set_code(code);
    pkt->set_key(key);
    memcpy(pkt->body(), data, len);

    // ENCRYPT BODY
    int r = pkt->encrypt_body(tx_key_);
    if (r != 0) {
        return {r, id_, CODE::E_INTERNAL, "marshal_n_enqueue(), encrypt"};
    }

    wpkts_.push_back(pkt);

    return net_msg::SUCCESS;
}


