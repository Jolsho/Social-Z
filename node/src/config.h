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

/////////////////////// //////////////////// ////////////////////

struct LogConfig {
    size_t      msgs_cap    { 32 };
};

/////////////////////// //////////////////// ////////////////////

struct FSConfig {
    size_t      msgs_cap    { 256 };
    size_t      map_size    { 10 * 1024 * 1024 };
};

struct DBConfig {
    size_t          msgs_cap        { 256 };
    size_t          map_size        { 10 * 1024 * 1024 };
};

/////////////////////// //////////////////// ////////////////////

struct HttpConfig {
    uint16_t    port;
    const char* ip;
};
HttpConfig* default_http_config(
    uint16_t    port        = 443,
    const char* ip          = "0.0.0.0"
);


struct TcpConfig {
    uint16_t    port;
    const char* ip;
};
TcpConfig* default_tcp_config(
    uint16_t    port,
    const char* ip
);

/////////////////////// //////////////////// ////////////////////

