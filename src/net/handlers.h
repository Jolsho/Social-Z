#pragma once
#include "net/net.h"

namespace handlers {
void handle_msg(net::Manager& netman, msg::Msg* pkt);
void handle_error(net::Manager& man, net_msg::Error e);
}
