#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "net.h"

int net::dial(const char* ip, unsigned short port) {
    int socket_fd;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) { return -1; }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(socket_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

void net::wipe_packet(net::Packet* pack) {
    return;
}

bool is_epollout_enabled(uint32_t events) {
    return (events & EPOLLOUT) != 0;
}

uint32_t enable_epollout(int epfd, int fd, uint32_t current_events) {
    struct epoll_event ev{};
    ev.data.fd = fd;
    ev.events = current_events | EPOLLOUT;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) return 0;
    return ev.events;
}

uint32_t disable_epollout(int epfd, int fd, uint32_t current_events) {
    struct epoll_event ev{};
    ev.data.fd = fd;
    ev.events = current_events & ~EPOLLOUT;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) return 0;
    return ev.events;
}

