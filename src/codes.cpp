#include "codes.h"

Actors code_too_too(CODE c) {
    if (c < CODE::P2P) 
        return Actors::NONE;

    else if (c < CODE::FS)
        return Actors::PEERNET;

    else if (c < CODE::RPC)
        return Actors::FILESYS;

    else if (c < CODE::BLOCK)
        return Actors::RPC_SERVER;

    else if (c < CODE::ERRORS)
        return Actors::BLOCKCHAIN;

    else return Actors::NONE;
}


