#include "api/actor.h"
#include "manager.h"
#include "lmdb.h"
#include "utils/path.h"
#include "utils/shutdown.h"
#include <cstdio>
#include <cstring>
#include <format>
#include <sys/epoll.h>
#include <thread>

ActorThread* start_fs(Actor* actor, FSConfig* conf) {
    ActorThread* at = new ActorThread{.r = 0};
    FS* fs = new FS(actor, conf);

    at->r = fs->initialize();
    if (at->r < 0) {
        delete fs;
        return at;
    }

    at->t = (void*)new std::thread([&] {
        fs->poll_loop();
        delete fs;
    });
    return at;
}

FS::FS(Actor* chans, FSConfig* conf) : 
    chans_(chans), 
    fs_root_(cpy_apnd(PATHS.data_dir, {"/fs/f_tree"}).c_str()),
    db_(cpy_apnd(PATHS.data_dir, {"/fs/lmdb"}).c_str(), conf->map_size),
    buffers_(BufferCaps{})
{

    path_.reserve(fs_root_.size() + HASH_SIZE * 2);
    path_.append(fs_root_);
    path_.resize(fs_root_.size() + HASH_SIZE * 2);

    open_files_.reserve(64);
    sessions_.reserve(64);


    static constexpr time_t LOG_FLUSH_INTERVAL = 500; // ms
    logr_ = new LogAccumulator{
        "FS", LOG_FLUSH_INTERVAL, 
        buffers_, ACTOR_FS
    };

}

int FS::initialize() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        logr_->log(std::format("EPOLL_CREATE1 FAILED: %d", epoll_fd_));
        return epoll_fd_;
    };
    return 0;
}

void FS::poll_loop() {
    const int MAX_EVENTS = 64;
    EventBuffer* events = new_event_buffer(MAX_EVENTS);

    while (true) {
        // short timeout
        poll_actor(chans_, events, in_msgs_, free_out_msgs_, 200);

        if (events->size < 0) {
            should_shutdown = true;
        }

        if (should_shutdown) {
            shutdown(); 
            return;
        }

        handle_internal_msgs();

        auto stats = poll_telemetry(chans_);
        if (stats != NULL) logr_->log(stats);

        time_t now = time(nullptr);
        if (pending_perms_.front().expires > now) {
            int r;
            HashT h;
            PendingPermBucket& ps = pending_perms_.front();
            auto txn = db_.start_txn();
            for (int del_cnt = 0; del_cnt < 12; del_cnt++) {
                ps.take_next(h);
                r = db_.del(h.b, HASH_SIZE, txn);
                if (
                    (r != 0 && r != MDB_NOTFOUND) || 
                    ps.is_empty()
                ) break;
            }
            db_.end_txn(txn, r);
            buffers_.put(ps.buff);
            pending_perms_.pop_front();
        }

        handle_outbound();
    }
}
