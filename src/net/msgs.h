#include "types.h"
#include "msg.h"
#include <string>

namespace net_msg {
struct Error {
    int         r       = -1;
    ConnID      id      = 0;
    CODE        code    = CODE::E_INTERNAL;
    std::string msg;

    inline bool is_err() const {
        return r != 0 || code != CODE::SUCCESS;
    }
};
const Error SUCCESS{0, 0, CODE::SUCCESS, ""};


void msg_close_conn(msg::Msg* msg, Actors from, ConnID id);
void msg_error(msg::Msg* msg, Error e, Actors from);
void msg_new_conn(
    msg::Msg* msg, 
    Actors from, 
    std::string ip,
    uint16_t port,
    Key &key
);
}
