
#include "msg.h"
#include "types.h"

Actors code_too_too(CODE c) {
    if (c < CODE::NET) 
        return Actors::NONE;

    else if (c < CODE::FS)
        return Actors::NETWORKER;

    else if (c < CODE::RPC)
        return Actors::FILESYS;

    else if (c < CODE::BLOCK)
        return Actors::RPC_SERVER;

    else if (c < CODE::ERRORS)
        return Actors::BLOCKCHAIN;

    else return Actors::NONE;
}
