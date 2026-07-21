/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz/api/actor.h"
#include "sz/p2p.h"
#include "manager.h"
#include <arpa/inet.h>
#include <sodium/crypto_box.h>
#include <sys/epoll.h>
#include "sz/utils/path.hpp"

static void* start(void* p) {
    ((P2P*)p)->poll_loop();
    delete (P2P*)p;
    return NULL;
};

ActorThread* start_p2p(Actor* actor, P2PConfig* conf) {
    ActorThread* at = new ActorThread{.r = 0};

    P2P* p2p = new P2P(actor, conf);
    at->r = p2p->start_server();
    if (at->r < 0) {
        delete p2p;
        return at;
    }

    pthread_create(&at->t, NULL, start, p2p);
    return at;
}

int P2P::start_server() {
    const std::string key_path = cpy_apnd(PATHS.config_dir, "/keys");
    citizens_.load(cpy_apnd(PATHS.data_dir, "/p2p/citizens"));

    int key_fd = open(key_path.c_str(), O_RDWR | O_CREAT, 0600);
    if (key_fd < 0) {
        log_msg(logr_, "OPEN KEY_FILE FAILED", key_fd, 0);
        return -1;
    }

    lseek(key_fd, 0, SEEK_SET);
    if (read(key_fd, keys_.priv.b, KEY_SIZE) < KEY_SIZE) {

        int r = crypto_box_keypair(keys_.pub.b, keys_.priv.b);
        if (r < 0) {
            log_msg(logr_, "GENERATING KEYS FAILED", r, 0);
            close(key_fd);
            remove(key_path.c_str());
            return -1;
        }

        lseek(key_fd, 0, SEEK_SET);
        if (write(key_fd, keys_.priv.b, KEY_SIZE) < KEY_SIZE) {
            close(key_fd);
            remove(key_path.c_str());
            log_msg(logr_, "PERSISTING KEYS FAILED", key_fd, 0);
            return -1;
        }
    }


    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    int r = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (r < 0) {
        log_msg(logr_, "SOCKET OPT FAILED", r, 0);
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
            log_msg(logr_, "INVALID IP", r, 0);
            return r;
        }
    }

    r = bind(listen_fd_, (sockaddr*)&addr, sizeof(addr));
    if (r < 0) {
        log_msg(logr_, "BIND FAILED", r, 0);
        return r;
    }

    r = listen(listen_fd_, SOMAXCONN);
    if (r < 0) {
        log_msg(logr_, "LISTEN FAILED", r, 0);
        return r;
    }
    
    int flags = fcntl(listen_fd_, F_GETFL, 0);
    if (flags < 0) {
        log_msg(logr_, "FCNTL FAILED", r, 0);
        return r;
    }

    r = fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK);
    if (r < 0) {
        log_msg(logr_, "SET FLAGS FAILED", r, 0);
        return r;
    }

    EpollEvent ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd_;
    r = ctl_epoll(chans_, &ev, EPOLL_CTL_ADD);
    if (r < 0) {
        log_msg(logr_, "EPOLL ADDING FAILED", r, 0);
        return r;
    }
    return 0;
}

void P2P::shutdown() {
    for (const auto& c: connections_) {
        remove_socket(c.id_);
    }
    close(listen_fd_);
    log_msg(logr_, "Server Shutdown Successful.", 0, 0);
    flush(logr_, this->free_out_msgs_);
}
