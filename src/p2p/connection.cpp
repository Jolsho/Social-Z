#include <format>
#include "msg.h"
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

Error conn::Connection::read_(
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
                        Code::E_OVERSIZED, 
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
                Code::E_MALFORMED, 
                std::format("read_() :: decrypt :: {}", 
                    key_to_str(remote_auth_key_)
                )
            };
        }

        if (status_ == conn::Status::Live) {
            Msg* msg = man.get_msg();

            msg->code = (Code)rpkt_.get_code();

            if (msg->code == Code::PING) {
                Error e = marshal_n_enqueue_msg(man.get_pkt(), man.keys_.pub, Code::PONG, NULL, 0);
                if (e.is_err()) {
                    return { 
                        -1, id_, 
                        Code::E_INTERNAL, 
                        std::format("read_() :: marshal_pong :: {}", 
                             key_to_str(remote_auth_key_)
                        )
                    };
                }
            } else if (msg-> code == Code::PONG) {

            }

            msg->too = code_too_too(static_cast<Code>(msg->code));

            // MAKE SURE ITS NOT A (CTRL || INTERNAL) Code
            if (msg->too == Actors::NONE && msg->code < Code::ERRORS) {
                msg_wipe(msg);
                man.msgs_.push_back(msg);
                return { 
                    -1, id_, 
                    Code::E_UNAUTHORIZED, 
                    std::format("read_() :: bad code :: {} :: {}", 
                         (uint16_t)msg->code , key_to_str(remote_auth_key_)
                    )
                };

            } else if (msg->code > Code::ERRORS) {
                // TODO handle error somehow

            } else {

                // ROUTE TO HANDLER THREAD
                msg->id = id_;
                msg_insert(msg, reinterpret_cast<uint8_t*>(rpkt_.body()), rpkt_.get_body_len());
                if (!man.to_main_.push(msg)) {
                    // TODO 
                }
            }

            rpkt_.wipe();

        } else if (status_ == conn::Status::CryptoSynAck) {
            return syn_ack(man);
        } else if (status_ == conn::Status::CryptoAck) {
            return ack(man);
        }
    }

    return ESUCCESS;
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

Error conn::Connection::marshal_n_enqueue_msg(
    Packet* pkt, 
    const Key& key,
    uint16_t code,
    std::byte* data,
    size_t len
) {
    if (wpkts_.size() >= wpkts_.capacity()) 
        return {-1, id_, Code::E_INTERNAL, "Write Buffer CAP reached."};

    pkt->set_len(len);
    pkt->set_version(version_);
    pkt->set_code(code);
    pkt->set_key(key);
    if (data)
        memcpy(pkt->body(), data, len);

    // ENCRYPT BODY
    int r = pkt->encrypt_body(tx_key_);
    if (r != 0) {
        return {r, id_, Code::E_INTERNAL, "marshal_n_enqueue(), encrypt"};
    }

    wpkts_.push_back(pkt);

    return ESUCCESS;
}


