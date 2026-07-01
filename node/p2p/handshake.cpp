#include "connection.h"
#include "sz/crypto.h"
#include <sodium/crypto_kx.h>


const size_t HANDSHAKE_LEN = 128;
Error conn::syn(Connection& conn, P2P& man) {

    // BUILD RESPONSE
    conn.wpkt_.buff_ = man.buffers_.grab(HANDSHAKE_LEN);

    conn.wpkt_.set_code(ACTOR_P2P);
    conn.wpkt_.set_version(conn.version_);
    conn.wpkt_.set_key(man.keys_.pub);
    conn.wpkt_.set_len(HANDSHAKE_LEN);
    vec_write(conn.wpkt_.buff_, conn.keys_.session_.pub.b, KEY_SIZE);

    // ENCRYPT RESPONSE
    int r = conn.wpkt_.encrypt_body(conn.keys_.tx_);
    if (r != 0) {
        man.buffers_.put(conn.wpkt_.buff_);
        return {r, conn.id_, E_INTERNAL, "syn(), encrypt"};
    }

    conn.status_ = conn::Status::CryptoAck;

    return ESUCCESS;
}

Error conn::syn_ack(Connection& conn, P2P& man) {

    // AUTHORIZE INCOMING CONNECTION
     conn.rpkt_.get_key(&conn.keys_.remote_auth_);
    auto it = man.citizens_.map_.find(conn.keys_.remote_auth_);
    if (it == man.citizens_.map_.end()) {
        return {
            .r = -1,
            .id = conn.id_,
            .code = E_BAD_ANON, 
            .key = conn.keys_.remote_auth_,
            .msg = "Not a citizen {}."
        };
    }
    Citizen& citizen = it->second;
    if (!citizen.is_trustworthy()) {
        return {
            .r = -1,
            .id = conn.id_,
            .code = E_BANNED, 
            .key = conn.keys_.remote_auth_,
            .msg = "Not Trustworthy Citizen."
        };
    }
    int r = crypto_kx_client_session_keys(
        conn.keys_.rx_.b, conn.keys_.tx_.b, 
        man.keys_.pub.b, 
        man.keys_.priv.b, 
        conn.keys_.remote_auth_.b
    );
    if (r != 0) {
        return {
            .r = r,
            .id = conn.id_, 
            .code = E_MALFORMED, 
            .key = conn.keys_.remote_auth_,
            .msg = "syn_ack() :: key ex",
        };
    }

    // UPDATE VERSION TO MATCH
    uint64_t version;
    conn.rpkt_.get_version(&version);
    if (version < conn.version_) {
        conn.version_ = version;
    }

    // BODY
    r = conn.rpkt_.decrypt_body(conn.keys_.rx_);
    if (r != 0) {
        return {
            .id = conn.id_, 
            .code = E_BAD_ANON, 
            .key = conn.keys_.remote_auth_,
            .msg = "syn_ack() :: decrypt body"
        };
    }

    // PARSE INCOMING SYN
    vec_read(conn.rpkt_.buff_, conn.keys_.remote_session_.b, KEY_SIZE);


    // BUILD RESPONSE
    conn.wpkt_.buff_ = man.buffers_.grab(HANDSHAKE_LEN);

    conn.wpkt_.set_code(ACTOR_P2P);
    conn.wpkt_.set_version(conn.version_);
    conn.wpkt_.set_key(man.keys_.pub);
    conn.wpkt_.set_len(HANDSHAKE_LEN);
    vec_write(conn.wpkt_.buff_, conn.keys_.session_.pub.b, KEY_SIZE);

    // ENCRYPT RESPONSE
    r = conn.wpkt_.encrypt_body(conn.keys_.tx_);
    if (r != 0) {
        man.buffers_.put(conn.wpkt_.buff_);
        return {r, conn.id_, E_INTERNAL, "syn_ack(), encrypt"};
    }


    // DERIVE FINAL SHARED KEYS AFTER SENDING SYNACK
    r = crypto_kx_client_session_keys(
        conn.keys_.rx_.b, conn.keys_.tx_.b, 
        conn.keys_.session_.pub.b, 
        conn.keys_.session_.priv.b, 
        conn.keys_.remote_session_.b
    );
    if (r != 0) {
        return {
            .r = r, .id = conn.id_,  
            .code = E_INTERNAL,
            .key = conn.keys_.remote_auth_,
            .msg = "syn_ack() :: key exchange", 
        };
    }

    conn.status_ = conn::Status::Live;

    char key[encoded_key_len()];
    key_to_str(key, &conn.keys_.remote_auth_);
    std::string msg;
    msg.reserve(10 + encoded_key_len());
    msg.append("NEW CONN: ");
    msg.append(key);
    man.logr_->log(msg);
    return ESUCCESS;
}

Error conn::ack(Connection& conn, P2P& man) {
    // PARSE INCOMING SYNACK

    uint8_t remote_version = vec_read<uint8_t>(conn.rpkt_.buff_);
    vec_read(conn.rpkt_.buff_, conn.keys_.remote_session_.b, KEY_SIZE);

    // ALTER STATE BASED ON SYN
    if (remote_version < conn.version_) {
        conn.version_ = remote_version;
    }

    // DERIVE SESSION SHARED KEYS AFTER SENDING SYNACK
    int r = crypto_kx_client_session_keys(
        conn.keys_.rx_.b, conn.keys_.tx_.b, 
        conn.keys_.session_.pub.b, 
        conn.keys_.session_.priv.b, 
        conn.keys_.remote_session_.b
    );
    if (r != 0) {
        return {
            .r = r, .id = conn.id_,  
            .code = E_MALFORMED,
            .key = conn.keys_.remote_auth_,
            .msg = "ack() :: key exchange"
        };
    }

    conn.status_ = conn::Status::Live;
    conn::enable_epollout(conn, man.chans_);

    char key[encoded_key_len()];
    key_to_str(key, &conn.keys_.remote_auth_);
    std::string msg;
    msg.reserve(10 + encoded_key_len());
    msg.append("NEW CONN: ");
    msg.append(key);
    man.logr_->log(msg);

    return ESUCCESS;
}

