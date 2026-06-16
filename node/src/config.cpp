#include "config.h"

HttpConfig* default_http_config(
    uint16_t    port,
    const char* ip
) {
    return new HttpConfig{
        .port = port,
        .ip = ip,
    };
}

TcpConfig* default_tcp_config(
    uint16_t    port,
    const char* ip
) {
    return new TcpConfig{
        .port = port,
        .ip = ip,
    };
}
