#include "http/server.h"
#include "codes.h"
#include "http/parse.h"
#include "llhttp.h"
#include "openssl/ssl.h"
#include <arpa/inet.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

Server::Server(msg::ChannelPair& chan, size_t msgs_cap, size_t pkts_cap) : 
    from_main_(chan.to), to_main_(chan.from), msgs_(msgs_cap)
{
    conns_.reserve(MAX_CONNECTIONS);

    settings_.on_message_begin = parse::on_message_begin;

    settings_.on_url = parse::on_url;
    settings_.on_url_complete = parse::on_url_complete;

    settings_.on_method = parse::on_method;
    settings_.on_method_complete = parse::on_method_complete;

    settings_.on_header_field = parse::on_header_field;
    settings_.on_header_field_complete = parse::on_header_complete;
    settings_.on_header_value = parse::on_header_value;
    settings_.on_header_value_complete = parse::on_header_complete;

    settings_.on_headers_complete = parse::on_headers_complete;
    settings_.on_body = parse::on_body;

    settings_.on_message_complete = parse::on_message_complete;
}

void Server::close_connection(conn_t& c) {
    delete static_cast<HandlerData*>(c.parser.data);
    conn_ids_.erase(c.fd);
    free_ids_.push_back(c.id);
    close(c.fd);
    return c.wipe();
}

int Server::start_server(const char* ip, ConnID port, const char* cert, const char* key) {
    ctx_ = SSL_CTX_new(TLS_server_method());
    if (!ctx_) {
        perror("epoll_create1");
        return -1;
    }

    if (SSL_CTX_use_certificate_file(ctx_, cert, SSL_FILETYPE_PEM) != 1) {
        perror("cert file failed init.");
        return -1;
    }
    if (SSL_CTX_use_PrivateKey_file(ctx_, key, SSL_FILETYPE_PEM) != 1) {
        perror("private key file failed init.");
        return -1;
    }

    if (SSL_CTX_check_private_key(ctx_) != 0) {
        perror("private key invalid.");
        return -1;
    };

    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        perror("epoll_create1");
        return -1;
    }

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        printf("SOCKET FAILURE :: %d\n", listen_fd_);
        return listen_fd_;
    }

    int opt = 1;
    int r = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (r < 0) {
        printf("BAD SOCKET OPT :: %d\n", r);
        return r;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (ip == nullptr) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        r = inet_pton(AF_INET, ip, &addr.sin_addr);
        if (r < 0) {
            close(listen_fd_);
            printf("NOT VALID IP :: %d\n", r);
            return r;
        }
    }

    r = bind(listen_fd_, (sockaddr*)&addr, sizeof(addr));
    if (r < 0) {
        printf("FAILED BIND :: %d\n", r);
        return r;
    }

    r = listen(listen_fd_, SOMAXCONN);
    if (r < 0) {
        printf("FAILED LISTEN :: %d\n", r);
        return r;
    }
    
    int flags = fcntl(listen_fd_, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl F_GETFL");
        return -1;
    }

    r = fcntl(listen_fd_, F_SETFL, flags | O_NONBLOCK);
    if (r < 0) {
        printf("FAILED FLAG SETTING :: %d\n", r);
        return r;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd_;

    r = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);
    if (r < 0) {
        printf("FAILED ADDING LIST TO EPOLL :: %d\n", r);
        return r;
    }
    return 0;
}

void Server::poll_loop() {
    const int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];

    while (1) {
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 0); // short timeout
        time_t now = time(nullptr);

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;

            if (fd == listen_fd_) {
                accept_new_connections(listen_fd_, epoll_fd_, ctx_);
                continue;

            } else if (fd == from_main_.get_event_fd()) {
                // INTERNAL MSGS
                int k = 0;
                while (msg::Msg* msg = from_main_.pop()) {
                    if (!msg->is_wiped && msg->code < CODE::ERRORS) {
                        handle_msg(msg);

                    } else if (!msg->is_wiped) {

                        // HANDLE ERROR MSG
                        net_msg::Error e {
                            .id     = msg->id,
                            .code   = (CODE)msg->code,
                        };

                        size_t size_r = sizeof(e.r);
                        if (msg->data.size() > size_r) {
                            memcpy(msg->data.data(), &e.r, size_r);
                            e.msg.resize(msg->data.size() - size_r);
                            if (e.msg.size() > 0) {
                                e.msg.copy(
                                    (char*)msg->data.data() + size_r, 
                                    msg->data.size() - size_r
                                );
                            }
                        }
                        handle_error(e);
                    }

                    if (!msg->is_wiped) msg->wipe();

                    if (msg->from == Actors::RPC_SERVER) {
                        if (msgs_.size() < msgs_.capacity()) {
                            msgs_.push_back(msg);
                        } else {
                            delete msg;
                        }
                    } else {
                        if (!to_main_.push(msg)) {
                            delete msg;
                        }
                    }

                    if (++k > 32) break;
                }
                continue;

            }


            // I think we can guarantee the connID exists.
            conn_t &c = conns_[conn_ids_[fd]];

            if (c.hand_failures >= 0) {
                int r = c.drive_tls_handshake();
                if (r != 0 && r != -1) {
                    close_connection(c);
                } else if (r == 0) {
                    c.hand_failures = -1;
                }
                continue;

            } else {
                if (events[i].events & EPOLLIN) {
                    if (c.err.is_ok()) {
                        if (c.read_() != 0)
                            c.queue_response("", nullptr, c.err);
                    }
                }

                if (events[i].events & EPOLLOUT) {
                    if (c.outbuf.cursor > 0)
                        if (c.write_() != 0 || (c.outbuf.cursor == 0 && !c.err.is_ok())) {
                            close_connection(c);
                            continue;
                        }

                    if (c.outbuf.cursor == 0) {
                        struct epoll_event ev{};
                        ev.data.fd = fd;
                        ev.events = c.events &= ~EPOLLOUT;
                        bool ok = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == 0;
                    }

                } else {
                    if (c.outbuf.cursor > 0) {
                        struct epoll_event ev{};
                        ev.data.fd = fd;
                        ev.events = c.events |= EPOLLOUT;
                        bool ok = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == 0;
                    }
                }
            }
        }
    }
}

void Server::accept_new_connections(int listen_fd, int epfd, SSL_CTX *ctx) {
    while (1) {
        sockaddr_storage addr;
        socklen_t len = sizeof(addr);

        int client_fd = accept4(
            listen_fd, (struct sockaddr*)&addr, 
            &len, SOCK_NONBLOCK
        );
        if (client_fd < 0) break;


        char ip_str[INET6_ADDRSTRLEN];
        int port;

        if (addr.ss_family == AF_INET) {
            // IPv4
            sockaddr_in* s = (sockaddr_in*)&addr;
            inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof(ip_str));
            port = ntohs(s->sin_port);

        } else if (addr.ss_family == AF_INET6) {
            // IPv6
            sockaddr_in6* s = (sockaddr_in6*)&addr;
            inet_ntop(AF_INET6, &s->sin6_addr, ip_str, sizeof(ip_str));
            port = ntohs(s->sin6_port);
        }

        // PREVENT BAD ACTORS
        if (banned_ips_.contains(ip_str)) {
            close(client_fd);
            continue;
        }

        // Allocate ID
        ConnID id;
        if (!free_ids_.empty()) {
            id = free_ids_.back();
            free_ids_.pop_back();
        } else {
            id = next_id_++;
        }

        conn_t &c = conns_[id];
        conn_ids_[client_fd] = id;

        c.id = id;
        c.fd = client_fd;
        c.ip = std::string{ ip_str };

        llhttp_init(&c.parser, HTTP_REQUEST, &settings_);
        c.parser.data = new HandlerData{ &c, this };

        c.ssl = SSL_new(ctx);
        c.events = EPOLLIN | EPOLLET;

        SSL_set_fd(c.ssl, client_fd);
        SSL_set_accept_state(c.ssl);


        epoll_event ev;
        ev.events = c.events;
        ev.data.fd = client_fd;

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
            close_connection(c);
        }
    }
}

void Server::handle_error(net_msg::Error e) {}
void Server::handle_msg(msg::Msg* m) {}

int Server::build_n_send_msg(conn_t& c) {

    msg::Msg* m = get_msg();

    std::byte* cursor = m->data.data();
    std::byte* start = cursor;

    for (auto& dest :PATHS) {
        if (dest.path != c.r.path && 
            dest.method == c.r.method
        ) continue;

        m->code = dest.code;
        m->too = dest.to;
        m->from = Actors::RPC_SERVER;
        m->id = c.id;


        size_t qcount = c.r.query.size();
        memcpy(cursor, &qcount, sizeof(size_t));
        cursor += sizeof(size_t);

        for (auto& [n, v] : c.r.query) {
            m->data.resize(m->data.size() + n.size() + v.size());
            memcpy(cursor, n.data(), n.size());
            cursor += n.size();
            memcpy(cursor, v.data(), v.size());
            cursor += v.size();

            if (cursor - start > QUERY_LEN) {
                c.err.status = Status::URI_TOO_LONG;
                c.err.reason = "Max Query Length Exceeded";
                return -1;
            }
        }
        break;
    }

    if (c.r.content_len > 0) {
        m->data.resize(m->data.size() + c.r.content_len);
        memcpy(cursor, c.inbuf.buff, c.inbuf.cursor);
    }

    if (!to_main_.push(m)) {
        m->wipe();
        msgs_.push_back(m);
        c.err.status = Status::SERVICE_UNAVAILABLE;
        c.err.reason = "Channel to handler is full.";
        return -1;
    }

    return 0;
}

