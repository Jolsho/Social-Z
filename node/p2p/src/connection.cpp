#include <format>
#include "p2p.h"
#include "connection.h"

void conn::Connection::clear() {
    fd_ = -1;
    id_ = 0;
    size_t key_s = keys_.rx_.size();
    memset(&keys_.remote_auth_, 0, key_s);
    memset(&keys_.remote_session_, 0, key_s);
    memset(&keys_.session_, 0, sizeof(KeyPair));
    memset(&keys_.rx_, 0, key_s);
    memset(&keys_.tx_, 0, key_s);
    events_ = 0;
    failure_count_ = 0;
    status_ = Status::Dead;

    rpkt_.buff_->len = 0;
    memset(rpkt_.buff_->b, 0, rpkt_.buff_->cap);
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
    BufferStore& buffs
) {
    while (true) {

        // READING PREFIX
        if (rpkt_.prefix_cursor_ < Packet::PREFIX_LEN) {
            size_t n = read(fd_, rpkt_.get_prefix_cursor(), Packet::PREFIX_LEN);
            if (n <= 0) break;
            rpkt_.prefix_cursor_ += n;

            if (rpkt_.prefix_cursor_ == Packet::PREFIX_LEN) {
                if (rpkt_.get_len() > Packet::MAX_LEN) {
                    rpkt_.wipe();
                    status_ = conn::Status::Failed;
                    return { -1, id_, 
                        E_OVERSIZED, 
                        keys_.remote_auth_,
                        std::format("read_() :: MAX Pkt size :: {}",
                            key_to_str(keys_.remote_auth_)
                        )
                    };
                }
            } else {
                break;
            }
        }


        // READING BODY
        uint64_t target = rpkt_.get_len();
        if (target > 0 && !rpkt_.buff_) {
            rpkt_.buff_ = buffs.grab(target);
        }
        if (rpkt_.body_cursor_ > Packet::PREFIX_LEN) {
            ssize_t n = read(fd_, rpkt_.get_body_cursor(), target - rpkt_.body_cursor_);
            if (n <= 0) break;
            rpkt_.body_cursor_ += n;
            if (rpkt_.body_cursor_ < target) break;
        }

        // DECRYPT BODY
        int r = rpkt_.decrypt_body(keys_.rx_);
        if (r != 0) {
            rpkt_.wipe();
            failure_count_++;

            buffs.put(rpkt_.buff_);

            return { r, id_, 
                E_MALFORMED, 
                keys_.remote_auth_,
                std::format("read_() :: decrypt :: {}", 
                    key_to_str(keys_.remote_auth_)
                )
            };
        }

        return ESUCCESS;
    }
    return ESUCCESS;
}


int conn::Connection::write_(p2p::Manager& netman) {
    if (!wpkt_.buff_ && pending_ids_.size() > 0) {
        int mid = pending_ids_.front();
        const Msg& m = netman.messenger_.get_msg(mid);

        if (m.code == p2p::code(p2p::Code::Ping)) {
            wpkt_.mark_as_ping();
            wpkt_.set_code(ACTOR_P2P);
        } else if (m.code == p2p::code(p2p::Code::Pong)) {
            wpkt_.mark_as_pong();
            wpkt_.set_code(ACTOR_P2P);
        } else {
            wpkt_.set_code(m.code);
        }

        wpkt_.set_key(netman.keys_.pub);
        wpkt_.set_version(version_);

        if (m.data->len > 0) {
            wpkt_.set_len(m.data->len);
            wpkt_.buff_ = netman.buffers_.grab(m.data->len);
            memcpy(wpkt_.buff_->b, m.data->b, m.data->len);
        }
    }

    unsigned char* cursor;
    uint64_t total;
    uint64_t* offset;

    if (wpkt_.prefix_cursor_ < Packet::PREFIX_LEN) {
        total = Packet::PREFIX_LEN;
        offset = &wpkt_.prefix_cursor_;
        cursor = wpkt_.get_prefix_cursor();
    } else {
        total = wpkt_.get_len();
        offset = &wpkt_.body_cursor_;
        cursor = wpkt_.get_body_cursor();
    }

    uint64_t written = write(fd_, cursor, total - *offset);
    if (written < 0 && (errno = EAGAIN || errno == EWOULDBLOCK)) {
        return -1;
    } else if (written < 0) {
        failure_count_++;
    }
    *offset += written;

    if (*offset == total) {
        netman.buffers_.put(wpkt_.buff_);
        wpkt_.buff_ = NULL;
        int mid = pending_ids_.front();
        pending_ids_.erase(pending_ids_.begin());

        wpkt_.wipe();

        if (pending_ids_.size() == 0) 
            disable_epollout(netman.epoll_fd_);

        return mid;
    }

    return -1;
}
