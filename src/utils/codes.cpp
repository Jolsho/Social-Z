#include "codes.h"

Actors code_too_too(Code c) {
    if (c < Code::P2P) 
        return Actors::NONE;

    else if (c < Code::FS)
        return Actors::PEERNET;

    else if (c < Code::RPC)
        return Actors::FILESYS;

    else if (c < Code::BLOCK)
        return Actors::RPC_SERVER;

    else if (c < Code::ERRORS)
        return Actors::BLOCKCHAIN;

    else return Actors::NONE;
}


