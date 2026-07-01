#include "sz/api/actor.h"
#include "manager.h"
#include "sz/utils/path.h"
#include "sz/utils/shutdown.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <sys/epoll.h>
#include <thread>

ActorThread* start_fs(Actor* actor, FSConfig* conf) {
    ActorThread* at = new ActorThread{.r = 0};
    FS* fs = new FS(actor, conf);

    at->t = (void*)new std::thread([&] {
        fs->poll_loop();
        delete fs;
    });
    return at;
}

FS::FS(Actor* chans, FSConfig* conf) : 
    chans_(chans), 
    fs_root_(cpy_apnd(PATHS.data_dir, {"/fs/f_tree"}).c_str()),
    db_( cpy_apnd(PATHS.data_dir,{"/fs/lmdb"}).c_str(), conf->map_size),
    buffers_(BufferCaps{})
{

    std::string tmp (fs_root_);
    tmp.append("/tmp");
    for (const auto& entry : std::filesystem::directory_iterator(tmp)) {
        std::filesystem::remove_all(entry.path());  // Recursively deletes files/directories
    }

    path_.reserve(fs_root_.size() + HASH_SIZE * 2);
    path_.append(fs_root_);
    path_.resize(fs_root_.size() + HASH_SIZE * 2);

    open_files_.reserve(conf->concurrent_sessions);
    sessions_.reserve(conf->concurrent_sessions);

    static constexpr time_t LOG_FLUSH_INTERVAL = 500; // ms
    logr_ = new LogAccumulator{
        "FS", LOG_FLUSH_INTERVAL, 
        buffers_, ACTOR_FS
    };

    allotted_space = conf->allotted_space;

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
            pending_perms_.pop_front();
        }

        while (
            session_expires_.size() > 0 && 
            std::get<0>(session_expires_.top()) < now
        ) {
            auto [exp, id] = session_expires_.top();

            auto it = sessions_.find(id);
            if (it != sessions_.end()) {
                Session& s = it->second;

                close(s.file->fd);
                open_files_.erase(s.file->hash);
                std::string path = derive_path(s.file->hash);
                remove(path.c_str());
                sessions_.erase(id);
            }
            session_expires_.pop();
        }

        handle_outbound();

        update_actor(chans_, &in_msgs_->cursor_, &free_out_msgs_->cursor_);
    }
}
