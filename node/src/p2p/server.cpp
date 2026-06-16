#include "p2p/p2p.h"
#include <arpa/inet.h>
#include <format>
#include <sodium/crypto_box.h>
#include "utils/path.h"

int p2p::Manager::start_server(P2PConfig& conf) {
    const std::string key_path = cpy_apnd(PATHS.config_dir, "/keys");
    citizens_.load(cpy_apnd(PATHS.data_dir, "/p2p/citizens"));

    int key_fd = open(key_path.c_str(), O_RDWR | O_CREAT, 0600);
    if (key_fd < 0) {
        logr_->log(std::format("OPEN KEY_FILE FAILED %d", key_fd));
        return -1;
    }

    lseek(key_fd, 0, SEEK_SET);
    if (read(key_fd, keys_.priv.data(), KEY_SIZE) < KEY_SIZE) {

        int r = crypto_box_keypair(keys_.pub.data(), keys_.priv.data());
        if (r < 0) {
            logr_->log(std::format("GENERATING KEYS FAILED %d", r));
            close(key_fd);
            remove(key_path.c_str());
            return -1;
        }

        lseek(key_fd, 0, SEEK_SET);
        if (write(key_fd, keys_.priv.data(), KEY_SIZE) < KEY_SIZE) {
            close(key_fd);
            remove(key_path.c_str());
            logr_->log("PERSISTING KEYS FAILED");
            return -1;
        }
    }


    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        logr_->log(std::format("EPOLL_CREATE1 FAILED %d", epoll_fd_));
        return -1;
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
    addr.sin_port = htons(conf.port);
    if (conf.ip == nullptr) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        r = inet_pton(AF_INET, conf.ip, &addr.sin_addr);
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

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd_;

    r = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);
    if (r < 0) {
        logr_->log(std::format("EPOLL ADDING FAILED: %d", r));
        return r;
    }
    return 0;
}

void p2p::Manager::shutdown() {
    for (const auto& c: connections_) {
        remove_socket(c.id_);
    }
    close(listen_fd_);
    logr_->log("Server Shutdown Successful.");
    logr_->flush(chans_);
}
