#include "p2p/protocols.h"
#include "p2p/p2p.h"

void marshal_ping(Msg &m) {
}

Error marshal_pong(BufferStore& buffs, Msg &msg, Vec* ping) {
    if (!ping) return {
        .r = -1, 
        .code = E_MALFORMED,
        .msg = "No ping provided to marshal pong."
    };

    msg.code = p2p::code(p2p::Code::Pong);
    msg.too = act_code(Actors::P2P);
    msg.from = act_code(Actors::P2P);
    msg.is_wiped = false;
    msg.priority = idx(Priority::Control);
    msg.data = buffs.grab(ping->len);
    vec_read(ping, msg.data);

    return ESUCCESS;
}
