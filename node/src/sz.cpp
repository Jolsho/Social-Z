#include <cassert>
#include "db/db.h"
#include "p2p/p2p.h"
#include "fs/fs.h"
#include "loop.h"
#include "utils/shutdown.h"
#include <sodium.h>

struct Connection {
    ConnID id;
    void reset();
};

class SZT {
public:
    std::array<ActorChannel, act_code(Actors::COUNT)>    actors_ = {
        ActorChannel{256, Actors::P2P},
        ActorChannel{256, Actors::FS},
        ActorChannel{256, Actors::SZ},
        ActorChannel{256, Actors::DB},
        ActorChannel{256, Actors::BC},
        ActorChannel{256, Actors::LOG, {8, 16, 8, 32}} // TELEMETRY HEAVY
    };
    std::array<std::thread, act_code(Actors::COUNT)>      threads_ = {};

    P2PConfig   p2p_config  = {};
    DBConfig    db_config    = {};
    FSConfig    fs_config   = {};

    std::vector<Connection>     connections_;
    ConnID                      next_id_;
    std::vector<ConnID>         free_ids_;

    void close_connection(ConnID id) {
        connections_[id].reset();
        free_ids_.push_back(id);
    }

    Connection& new_conection() {
        ConnID id;
        if (!free_ids_.empty()) {
            id = free_ids_.back();
            free_ids_.pop_back();
        } else {
            id = next_id_++;
        }
        return connections_[id];
    }
};
    

SZT* start_sz() {
    SZT* sz = new SZT();

    assert(sodium_init() != -1); 
    int main_epoll_fd = epoll_create1(0);

    // P2P
    register_queue(main_epoll_fd, sz->actors_[act_code(Actors::P2P)]);
    sz->threads_[act_code(Actors::P2P)] = std::thread([&]{
        p2p::Manager p2p_mgr(sz->actors_[act_code(Actors::P2P)], sz->p2p_config);
        assert(p2p_mgr.start_server(sz->p2p_config) == 0);
        p2p_mgr.poll_loop(); 
    });

    // FS
    register_queue(main_epoll_fd, sz->actors_[act_code(Actors::FS)]);
    sz->threads_[act_code(Actors::FS)] = std::thread([&]{
        fs::Manager file_mgr(sz->actors_[act_code(Actors::FS)], sz->fs_config);
        file_mgr.poll_loop(); 
    });

    // DB
    register_queue(main_epoll_fd, sz->actors_[act_code(Actors::DB)]);
    sz->threads_[act_code(Actors::DB)] = std::thread([&]{
        db::Server db_server(sz->actors_[act_code(Actors::DB)], sz->db_config);
        db_server.poll_loop(); 
    });

    // MAIN
    sz->threads_[act_code(Actors::COUNT)] = std::thread([&]{
        main_loop(main_epoll_fd, sz->actors_);
    });

    return sz;
}

void block_stop(SZT* sz) {
    should_shutdown = true;;
    for (auto& t: sz->threads_) t.join();
}
