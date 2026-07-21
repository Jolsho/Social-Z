/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz/api/paths.h"
#include "connection.h"
#include "manager.h"
#include "pkt.h"
#include "sz/crypto.h"
#include "sz/utils/error.h"
#include <format>

Error P2P::writeable_conn(Connection& conn) {
    int mid = conn::write_(conn, *this);
    if (mid >= 0) {
        while (true) {
            auto next = messenger_.next(mid);

            if (next.has_value()) {
                auto [key, msg] = next.value();
                
                ConnID id;
                if (connect(key, &id).has_value()) {
                    //messenger_.push_to_retry(mid, key);
                    continue;
                }

                Connection &conn = connections_[id];

                if (conn.pending_ids_.size() >= conn::MAX_PENDING_OUT)  {
                    // TODO messenger_.push_to_retry(mid, key);
                    continue;
                }
                conn.pending_ids_.push_back(mid);

                if (conn.status_ == conn::Status::Live && !conn::is_epollout_enabled(conn)) {
                        conn.events_ = conn::enable_epollout(conn, chans_);
                }
                break;

            } else {

                auto m = messenger_.remove(mid);
                if (!m.has_value()) break;

                if (m->data) {
                    if (m->from != ACTOR_P2P) {
                        m->priority = PRIORITY_CONT;
                        *consume_msg(free_out_msgs_) = m.value();
                    } else {
                        put_buff(buffers_, m->data);
                    }
                }
                break;
            }
        }
    }
    return ESUCCESS;
}

Error P2P::readable_conn(Connection& conn) {

    Error e = conn::read_(conn, buffers_); 

    if (!is_err(&e) && conn.rpkt_.is_done() && conn.status_ == conn::Status::Live) {

        PktCode code;
        conn.rpkt_.get_code(&code);

        Actors too;
        conn.rpkt_.get_too(&too);

        uint64_t len;
        conn.rpkt_.get_len(&len);

        // MAKE SURE ITS NOT A (CTRL || INTERNAL) Code
        if (too >= ACTOR_COUNT) {
            e = Error{ 
                .r = -1, 
                .id = conn.id_, 
                .code = E_UNAUTHORIZED, 
                .key = conn.keys_.remote_auth_,
                .msg = std::format("read_() :: bad too code :: {}", code).c_str()
            };

        } else if (too == ACTOR_P2P) {
            e = p2p_protocols(conn);

        } else if (Msg* msg = consume_msg(free_out_msgs_)) {
            msg->priority = PRIORITY_WORK;
            msg->too = too;
            msg->code = code;
            msg->data = grab_buff(buffers_, len);

            // ROUTE TO HANDLER THREAD
            msg->id = conn.id_;
            vec_write(msg->data, conn.rpkt_.get_body_cursor(), len);
        } else {
            e = { 
                -1, conn.id_, 
                E_INTERNAL, 
                conn.keys_.remote_auth_,
                "connection::read_() :: to_main_ is full."
            };
        }
    } else if (conn.status_ == conn::Status::CryptoSynAck) {
        e = conn::syn_ack(conn, *this);

    } else if (conn.status_ == conn::Status::CryptoAck) {
        e = conn::ack(conn, *this);
    }

    if (is_err(&e) || conn.rpkt_.is_done()) {
        conn.rpkt_.wipe();
        put_buff(buffers_, conn.rpkt_.buff_);
    }

    return e;
}

Error P2P::p2p_protocols(Connection& c) {
    int mid = messenger_.new_msg(NULL);
    Msg& msg = messenger_.get_mut_msg(mid);
    Error e = ESUCCESS;

    PktCode code;
    c.rpkt_.get_code(&code);

    if (code == P2P_PING) {

        msg.code = P2P_PONG;
        msg.too = ACTOR_P2P;
        msg.from = ACTOR_P2P;
        msg.is_wiped = false;
        msg.priority = PRIORITY_WORK;
        msg.data = grab_buff(buffers_, c.rpkt_.buff_->len);
        vec_read(c.rpkt_.buff_, msg.data, msg.data->len);

    } else if (code == P2P_PONG) {
        // TODO -- record
    }

    if (is_err(&e)) {
        auto r = messenger_.remove(mid);
        if (r.has_value()) {
            Msg& m = r.value();
            put_buff(buffers_, m.data);
        }
        return e;
    }
    c.pending_ids_.push_back(mid);

    return ESUCCESS;
}


void P2P::handle_error(Error e) {
    if (key_is_zero(&e.key)) {
        e.key = connections_[e.id].keys_.remote_auth_;
    }
    // TODO --> if E == E_BAD_ANON record IP
    // if IP happens multiple times then ban IP
    // using the ip socet or whatever...
    // this shit should be done in citizen I think.
    // we just make sure to pass the ip and key and error


    Citizen& citizen = citizens_.map_.at(e.key);
    citizen.record_infringement(e.code);

    if (!citizen.is_trustworthy()) {
        if (e.id == 0) {
            auto it =  key_to_conn_.find(e.key);
            if (it != key_to_conn_.end())
                remove_socket(it->second);
        } else {
            remove_socket(e.id);
        }
    }
    log_msg(logr_, e.msg, e.r, e.code);
}

Error P2P::handle_msg(Msg* msg) {
    if (!msg->is_wiped && msg->code >= E_SUCCESS) {
        // REGULAR INTERNAL MSG
        switch (msg->code) {
            case P2P_CLOSE_CONN: {
                remove_socket(msg->id);
                break;
            }

            case P2P_NEW_CONN: {
                Key pubkey;
                vec_read(msg->data, pubkey.b, KEY_SIZE);

                ConnID id;
                auto res = connect(pubkey, &id);
                if (res.has_value() && is_err(&res.value())) return res.value();
                
                break;
            }

            case P2P_BROADCAST: {
                int mid = messenger_.new_msg(msg);
                auto next = messenger_.next(mid);
                if (!next.has_value()) {
                    // This should never happen.
                    auto m = messenger_.remove(mid);
                    if (m.has_value()) {
                        Msg& mm = m.value();
                        mm.too = mm.from;
                        mm.priority = PRIORITY_CONT;
                        *consume_msg(free_out_msgs_) = mm;
                    }
                    return Error{ .r = -1, .msg = "Failed Broadcast" };
                }

                auto [key, _] = next.value();
                ConnID id = msg->id;

                Connection &conn = connections_[id];
                if (memcmp(conn.keys_.remote_auth_.b, key.b, KEY_SIZE) != 0) {
                    auto r = connect(key, &id);
                    if (r.has_value() && is_err(&r.value())) return r.value();
                    conn = connections_[id];
                }

                if (conn.pending_ids_.size() >= conn::MAX_PENDING_OUT)  {
                    return {-1,  conn.id_, E_INTERNAL, conn.keys_.remote_auth_, "Write Buffer CAP reached."};
                }
                conn.pending_ids_.push_back(mid);

                if (conn.status_ == conn::Status::Live && !conn::is_epollout_enabled(conn)) {
                        conn.events_ = conn::enable_epollout(conn, chans_);
                }
                /*
                    TODO -- 
                        need a way to be able to broadcast multiple chunks.

                        also we shouldnt be sending sequentially.
                        it should be a few at a time.

                        When they finally do connect or whatever:
                            - check for broadcast ID.
                            - get and marshal msg from broadcaster.
                                - make sure not already sending msg.
                                - if so wait until that finishes.
                            - keep track of msg idx(pack broadcast_id with an idx)
                            - if last pkt try to lead next recipient
                                - if no more and your last sending destroy broadcaster
                                    - this might not be right...
                                        - because of multiple chunks
                */
                break;
            }

            default: {
                return {-1,  0, E_INTERNAL, "Invalid msg code."};
                break;
            }
        }

    } else if (!msg->is_wiped) {

        // INTERNAL ERROR MSG
        Error e {};
        unmarshal_error(&e, msg);
        handle_error(e);
    }

    if (!msg->is_wiped) msg_wipe(msg);

    if (msg->from == ACTOR_P2P && msg->data) {

        put_buff(buffers_, msg->data);

    } else if (msg->from != ACTOR_P2P && msg->data) {
        msg->priority = PRIORITY_CONT;
        *consume_msg(free_out_msgs_) = *msg;
    }
    return ESUCCESS;
}
