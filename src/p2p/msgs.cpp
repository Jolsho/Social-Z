#include "p2p/msgs.h"
#include <cstring>

void net_msg::msg_close_conn(msg::Msg* msg, Actors from, ConnID id) {
    msg->too = Actors::PEERNET;
    msg->code = CODE::CLOSE_CONN;
    msg->from = from;
    msg->id = id;
}

void net_msg::msg_error(msg::Msg* msg, net_msg::Error e, Actors from) {
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

void net_msg::msg_new_conn(
    msg::Msg* msg, 
    Actors from, 
    std::string ip,
    uint16_t port,
    Key &key
) {
    msg->code = CODE::NEW_CONN;
    msg->id = 0;
    msg->too = Actors::PEERNET;
    msg->from = from;
    msg->is_wiped = false;

    msg->data.resize(1 + ip.size() + sizeof(uint16_t) + KEY_SIZE);
    auto cursor = msg->data.data();

    memcpy(cursor, ip.data(), ip.size());
    memcpy(cursor, &port, sizeof(uint16_t));
    memcpy(cursor, &key, KEY_SIZE);
}
