#pragma once
#include "utils/buffers.h"
#include "utils/error.h"

void marshal_ping(Msg &m);
Error marshal_pong(BufferStore& buffs, Msg &msg, Vec* ping);
