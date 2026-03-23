#include "net/net.h"
#include <format>
#include <sodium/crypto_kx.h>

const size_t HANDSHAKE_LEN = 128;
const size_t KEY_LEN = sizeof(Key);
net_msg::Error conn::Connection::syn(net::Manager& man) {

    std::byte* data[HANDSHAKE_LEN];
    std::byte* cursor = data[0];

    cpy_from_cursor(cursor, &session_keys_.pub);

    Packet* pkt = man.get_pkt();

    net_msg::Error e = marshal_n_enqueue_msg(pkt, man.keys_.pub, CODE::SYN, data[0], HANDSHAKE_LEN);
    if (e.is_err()) {
        man.pkts_.push_back(pkt);
        return e;
    }

    status_ = conn::Status::CryptoAck;

    return net_msg::SUCCESS;
}

net_msg::Error conn::Connection::syn_ack(net::Manager& man) {

    // AUTHORIZE INCOMING CONNECTION
    remote_auth_key_ = rpkt_.get_key();
    auto it = man.citizens_.find(remote_auth_key_);
    if (it == man.citizens_.end()) {
        return {
            .msg = std::format("Not a citizen {}.", key_to_str(remote_auth_key_))
        };
    }
    Citizen& citizen = it->second;
    if (!citizen.is_trustworthy()) {
        return {
            .msg = std::format("Not Trustworth Citizen {}.", key_to_str(remote_auth_key_))
        };
    }
    int r = crypto_kx_client_session_keys(
        rx_key_.data(), tx_key_.data(), 
        man.keys_.pub.data(), 
        man.keys_.priv.data(), 
        remote_auth_key_.data()
    );
    if (r != 0) {
        return {
            .id = id_, 
            .code = CODE::E_MALFORMED, 
            .msg = std::format("syn_ack() :: key ex :: {}",
                key_to_str(remote_auth_key_)
            )
        };
    }

    // UPDATE VERSION TO MATCH
    if (rpkt_.get_version() < version_) {
        version_ = rpkt_.get_version();
    }

    // HEADER
    if (rpkt_.get_code() != CODE::SYN) {
        return {
            .id = id_, 
            .code = CODE::E_UNAUTHORIZED, 
            .msg = std::format("syn_ack() :: invalid pkt code :: {}",
                key_to_str(remote_auth_key_)
            )
        };
    }

    // BODY
    r = rpkt_.decrypt_body(rx_key_);
    if (r != 0) {
        return {
            .id = id_, 
            .code = CODE::E_UNAUTHORIZED, 
            .msg = std::format("syn_ack() :: decrypt body :: {}",
                key_to_str(remote_auth_key_)
            )
        };
    }
    // PARSE INCOMING SYN
    std::byte* body = rpkt_.body();

    cpy_from_cursor(body, &remote_session_key_);


    // BUILD RESPONSE AND MARSHAL
    std::byte* data[HANDSHAKE_LEN];
    std::byte* cursor = data[0];

    cpy_from_cursor(cursor, &session_keys_.pub);

    Packet* pkt = man.get_pkt();
    net_msg::Error e = marshal_n_enqueue_msg(pkt, man.keys_.pub, CODE::SYNACK, data[0], HANDSHAKE_LEN);
    if (e.is_err()) {
        man.pkts_.push_back(pkt);
        return e;
    }


    // DERIVE FINAL SHARED KEYS AFTER SENDING SYNACK
    r = crypto_kx_client_session_keys(
        rx_key_.data(), tx_key_.data(), 
        session_keys_.pub.data(), 
        session_keys_.priv.data(), 
        remote_session_key_.data()
    );
    if (r != 0) {
        return {
            .r = r, .id = id_,  
            .msg = std::format(
                "syn_ack() :: key exchange :: {}.", 
                key_to_str(remote_auth_key_)
            )
        };
    }

    status_ = conn::Status::Live;

    return net_msg::SUCCESS;
}

net_msg::Error conn::Connection::ack(net::Manager& man) {
    // PARSE INCOMING SYNACK
    std::byte* body = rpkt_.body();

    uint8_t remote_version = read_from_cursor<uint8_t>(body);

    cpy_from_cursor(body, &remote_session_key_);

    // ALTER STATE BASED ON SYN
    if (remote_version < version_) {
        version_ = remote_version;
    }

    // DERIVE SESSION SHARED KEYS AFTER SENDING SYNACK
    int r = crypto_kx_client_session_keys(
        rx_key_.data(), tx_key_.data(), 
        session_keys_.pub.data(), 
        session_keys_.priv.data(), 
        remote_session_key_.data()
    );
    if (r != 0) {
        return {
            .r = r, .id = id_,  
            .msg = std::format(
                "ack() :: key exchange :: {}.", 
                key_to_str(remote_auth_key_)
            )
        };
    }

    status_ = conn::Status::Live;

    return net_msg::SUCCESS;
}

