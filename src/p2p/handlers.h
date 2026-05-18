#pragma once
#include "p2p/p2p.h"

namespace handlers {
Error handle_msg(p2p::Manager& netman, Msg* pkt);
void handle_error(p2p::Manager& man, Error e);
}
