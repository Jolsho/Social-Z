#pragma once
#include "p2p/p2p.h"

namespace handlers {
void handle_msg(p2p::Manager& netman, msg::Msg* pkt);
void handle_error(p2p::Manager& man, net_msg::Error e);
}
