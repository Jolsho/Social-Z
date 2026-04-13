
#include "crypto.h"
#include "msg.h"
#include <cstring>

void msg::p2p_close_conn(msg::Msg* msg, Actors from, ConnID id) {
    msg->too = Actors::PEERNET;
    msg->code = Code::CLOSE_CONN;
    msg->from = from;
    msg->id = id;
}

void msg::p2p_error(msg::Msg* msg, msg::Error e, Actors from) {
    msg->code = e.code;
    msg->id = e.id;
    msg->too = Actors::PEERNET;
    msg->from = from;
    msg->is_wiped = false;
    msg->data.resize(sizeof(e.r) + e.msg.size());
    auto cursor = msg->data.data();
    memcpy(cursor, &e.r, sizeof(e.r));
    memcpy(cursor, e.msg.data(), e.msg.size());
}

int msg::p2p_new_conn(
    msg::Msg* msg, 
    std::string ip,
    uint16_t port,
    std::string& key_str
) {
    Key key;
    if (str_to_key(key_str.data(), key) < 0) return -1;

    msg->code = Code::NEW_CONN;
    msg->id = 0;
    msg->too = Actors::PEERNET;
    msg->is_wiped = false;

    msg->data.resize(1 + ip.size() + sizeof(uint16_t) + KEY_SIZE);
    auto cursor = msg->data.data();

    memcpy(cursor, ip.data(), ip.size());
    memcpy(cursor, &port, sizeof(uint16_t));
    memcpy(cursor, key.data(), KEY_SIZE);

    return 0;
}
