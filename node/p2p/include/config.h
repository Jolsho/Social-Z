#pragma once
#include <cstddef>
#include <cstdint>

struct P2PConfig {
    size_t      msgs_cap    { 256 };
    size_t      pkts_cap    { 256 };
    uint16_t    port        { 3213 };
    const char* ip          { "0.0.0.0" };

    size_t      wave        { 25 };
    size_t      broad_msgs  { 128 }; 
};

