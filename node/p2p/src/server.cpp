#include "api/actor.h"
#include "manager.h"
#include <arpa/inet.h>
#include <format>
#include <sodium/crypto_box.h>
#include <sys/epoll.h>
#include <thread>
#include "utils/path.h"

ActorThread* start_p2p(Actor* actor, P2PConfig* conf) {
    ActorThread* at = new ActorThread{.r = 0};

    P2P* p2p = new P2P(actor, conf);
    at->r = p2p->start_server();
    if (at->r < 0) {
        delete p2p;
        return at;
    }
    at->t = (void*)new std::thread([&] {
        p2p->poll_loop();
        delete p2p;
    });

    return at;
}

int P2P::start_server() {
    const std::string key_path = cpy_apnd(PATHS.config_dir, "/keys");
    citizens_.load(cpy_apnd(PATHS.data_dir, "/p2p/citizens"));

    int key_fd = open(key_path.c_str(), O_RDWR | O_CREAT, 0600);
    if (key_fd < 0) {
        logr_->log(std::format("OPEN KEY_FILE FAILED %d", key_fd));
        return -1;
    }

    lseek(key_fd, 0, SEEK_SET);
    if (read(key_fd, keys_.priv.b, KEY_SIZE) < KEY_SIZE) {

        int r = crypto_box_keypair(keys_.pub.b, keys_.priv.b);
        if (r < 0) {
            logr_->log(std::format("GENERATING KEYS FAILED %d", r));
            close(key_fd);
            remove(key_path.c_str());
            return -1;
        }

        lseek(key_fd, 0, SEEK_SET);
        if (write(key_fd, keys_.priv.b, KEY_SIZE) < KEY_SIZE) {
            close(key_fd);
            remove(key_path.c_str());
            logr_->log("PERSISTING KEYS FAILED");
            return -1;
        }
    }


    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    int r = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (r < 0) {
        logr_->log(std::format("SOCKET OPT FAILED: %d", r));
        return r;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(conf_->port);
    if (conf_->ip == nullptr) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        r = inet_pton(AF_INET, conf_->ip, &addr.sin_addr);
        if (r < 0) {
            close(listen_fd_);
            logr_->log(std::format("INVALID IP: %d", r));
            return r;
        }
    }

    r = bind(listen_fd_, (sockaddr*)&addr, sizeof(addr));
    if (r < 0) {
        logr_->log(std::format("BIND FAILED: %d", r));
        return r;
    }

    r = listen(listen_fd_, SOMAXCONN);
    if (r < 0) {
        logr_->log(std::format("LISTEN FAILED: %d", r));
        return r;
    }
    
    int flags = fcntl(listen_fd_, F_GETFL, 0);
    if (flags < 0) {
        logr_->log(std::format("FCNTL FAILED: %d", r));
        return r;
    }

    r = fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK);
    if (r < 0) {
        logr_->log(std::format("SET FLAGS FAILED: %d", r));
        return r;
    }

    EpollEvent ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd_;
    r = ctl_epoll(chans_, &ev, EPOLL_CTL_ADD);
    if (r < 0) {
        logr_->log(std::format("EPOLL ADDING FAILED: %d", r));
        return r;
    }
    return 0;
}

void P2P::shutdown() {
    for (const auto& c: connections_) {
        remove_socket(c.id_);
    }
    close(listen_fd_);
    logr_->log("Server Shutdown Successful.");
    logr_->flush(this->free_out_msgs_);
}
