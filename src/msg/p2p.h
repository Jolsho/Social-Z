#include "codes.h"
#include "msg.h"
#include "error.h"

void p2p_close_conn(Msg* msg, Actors from, ConnID id);
void p2p_error(Msg* msg, Error e, Actors from);
int p2p_new_conn(
    Msg* msg, 
    std::string ip,
    uint16_t port,
    std::string& key
);
int p2p_ping(
    Msg* msg, 
    std::string ip,
    uint16_t port,
    std::string& key_str
);
