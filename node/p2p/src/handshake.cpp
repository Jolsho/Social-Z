#include "p2p.h"
#include "utils/vec.h"
#include <cstring>
#include <format>
#include <sodium/crypto_kx.h>


const size_t HANDSHAKE_LEN = 128;
Error conn::Connection::syn(p2p::Manager& man) {

    // BUILD RESPONSE
    wpkt_.buff_ = man.buffers_.grab(HANDSHAKE_LEN);

    wpkt_.set_code(ACTOR_P2P);
    wpkt_.set_version(version_);
    wpkt_.set_key(man.keys_.pub);
    wpkt_.set_len(HANDSHAKE_LEN);
    vec_write(wpkt_.buff_, keys_.session_.pub.data(), KEY_SIZE);

    // ENCRYPT RESPONSE
    int r = wpkt_.encrypt_body(keys_.tx_);
    if (r != 0) {
        man.buffers_.put(wpkt_.buff_);
        return {r, id_, E_INTERNAL, "syn(), encrypt"};
    }

    status_ = conn::Status::CryptoAck;

    return ESUCCESS;
}

Error conn::Connection::syn_ack(p2p::Manager& man) {

    // AUTHORIZE INCOMING CONNECTION
    keys_.remote_auth_ = rpkt_.get_key();
    auto it = man.citizens_.map_.find(keys_.remote_auth_);
    if (it == man.citizens_.map_.end()) {
        return {
            .r = -1,
            .id = id_,
            .code = E_BAD_ANON, 
            .msg = std::format("Not a citizen {}.", key_to_str(keys_.remote_auth_))
        };
    }
    Citizen& citizen = it->second;
    if (!citizen.is_trustworthy()) {
        return {
            .r = -1,
            .id = id_,
            .code = E_BANNED, 
            .msg = std::format("Not Trustworthy Citizen {}.", key_to_str(keys_.remote_auth_))
        };
    }
    int r = crypto_kx_client_session_keys(
        keys_.rx_.data(), keys_.tx_.data(), 
        man.keys_.pub.data(), 
        man.keys_.priv.data(), 
        keys_.remote_auth_.data()
    );
    if (r != 0) {
        return {
            .r = r,
            .id = id_, 
            .code = E_MALFORMED, 
            .msg = std::format("syn_ack() :: key ex :: {}",
                key_to_str(keys_.remote_auth_)
            )
        };
    }

    // UPDATE VERSION TO MATCH
    if (rpkt_.get_version() < version_) {
        version_ = rpkt_.get_version();
    }

    // BODY
    r = rpkt_.decrypt_body(keys_.rx_);
    if (r != 0) {
        return {
            .id = id_, 
            .code = E_BAD_ANON, 
            .msg = std::format("syn_ack() :: decrypt body :: {}",
                key_to_str(keys_.remote_auth_)
            )
        };
    }

    // PARSE INCOMING SYN
    vec_read(rpkt_.buff_, keys_.remote_session_.data(), KEY_SIZE);


    // BUILD RESPONSE
    wpkt_.buff_ = man.buffers_.grab(HANDSHAKE_LEN);

    wpkt_.set_code(ACTOR_P2P);
    wpkt_.set_version(version_);
    wpkt_.set_key(man.keys_.pub);
    wpkt_.set_len(HANDSHAKE_LEN);
    vec_write(wpkt_.buff_, keys_.session_.pub.data(), KEY_SIZE);

    // ENCRYPT RESPONSE
    r = wpkt_.encrypt_body(keys_.tx_);
    if (r != 0) {
        man.buffers_.put(wpkt_.buff_);
        return {r, id_, E_INTERNAL, "syn_ack(), encrypt"};
    }


    // DERIVE FINAL SHARED KEYS AFTER SENDING SYNACK
    r = crypto_kx_client_session_keys(
        keys_.rx_.data(), keys_.tx_.data(), 
        keys_.session_.pub.data(), 
        keys_.session_.priv.data(), 
        keys_.remote_session_.data()
    );
    if (r != 0) {
        return {
            .r = r, .id = id_,  
            .code = E_INTERNAL,
            .msg = std::format(
                "syn_ack() :: key exchange :: {}.", 
                key_to_str(keys_.remote_auth_)
            )
        };
    }

    status_ = conn::Status::Live;
    man.logr_->log(std::format("NEW CONN: %s", key_to_str(keys_.remote_auth_)));

    return ESUCCESS;
}

Error conn::Connection::ack(p2p::Manager& man) {
    // PARSE INCOMING SYNACK

    uint8_t remote_version = vec_read<uint8_t>(rpkt_.buff_);
    vec_read(rpkt_.buff_, keys_.remote_session_.data(), KEY_SIZE);

    // ALTER STATE BASED ON SYN
    if (remote_version < version_) {
        version_ = remote_version;
    }

    // DERIVE SESSION SHARED KEYS AFTER SENDING SYNACK
    int r = crypto_kx_client_session_keys(
        keys_.rx_.data(), keys_.tx_.data(), 
        keys_.session_.pub.data(), 
        keys_.session_.priv.data(), 
        keys_.remote_session_.data()
    );
    if (r != 0) {
        return {
            .r = r, .id = id_,  
            .code = E_MALFORMED,
            .msg = std::format(
                "ack() :: key exchange :: {}.", 
                key_to_str(keys_.remote_auth_)
            )
        };
    }

    status_ = conn::Status::Live;
    enable_epollout(man.epoll_fd_);

    man.logr_->log(std::format("NEW CONN: %s", key_to_str(keys_.remote_auth_)));

    return ESUCCESS;
}

