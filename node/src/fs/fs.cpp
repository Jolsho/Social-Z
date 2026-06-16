#include "fs/fs.h"
#include "lmdb.h"
#include "utils/chans.h"
#include "utils/error.h"
#include "utils/path.h"
#include "utils/shutdown.h"
#include "utils/vec.h"
#include <cstdio>
#include <cstring>
#include <format>

fs::Manager::Manager(ActorChannel& chans, FSConfig& conf) : 
    chans_(chans), 
    fs_root_(cpy_apnd(PATHS.data_dir, {"/fs/f_tree"}).c_str()),
    db_(cpy_apnd(PATHS.data_dir, {"/fs/lmdb"}).c_str(), conf.map_size),
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
        buffers_, Actors::FS
    };

}

int fs::Manager::initialize() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        logr_->log(std::format("EPOLL_CREATE1 FAILED: %d", epoll_fd_));
        return epoll_fd_;
    };
    return 0;
}

void fs::Manager::poll_loop() {
    const int MAX_EVENTS = 64;
    epoll_event events[MAX_EVENTS];
    while (true) {
        // short timeout
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 0); 
        time_t now = time(nullptr);

        if (should_shutdown) {
            shutdown(); 
            return;
        }
        auto stats = chans_.poll_telemetry();
        if (stats) {
            logr_->log(stats);
        }

        for (int i = 0; i < n; ++i) {

            int fd = events[i].data.fd;
            if (fd == chans_.get_event_fd()) {
                chans_.clear_event();

                int k = 0;
                Msg* msg;
                while (chans_.in_.front(msg)) {
                    if (!msg->is_wiped && msg->data && msg->data->len > sizeof(FSCODE)) {

                        // If no space just silently drop
                        if (chans_.out_.has_space(Priority::Work)) {

                            Error e { 
                                .id = msg->id 
                            };


                            if (msg->from == act_code(Actors::P2P)) {
                                switch (vec_read<FSCODE>(msg->data)) {
                                    case FSCODE::VOUCHER:   voucher(msg, e);   break;
                                    case FSCODE::REDEEM:    redeem(msg, e);    break;
                                    case FSCODE::REWARD:    reward(msg, e);    break;

                                    case FSCODE::GIVE:      give(msg, e);      break;
                                    case FSCODE::SETTLE:    settle(msg, e);    break;
                                    case FSCODE::ASK:       ask(msg, e);       break;
                                    case FSCODE::REVOKE:    revoke(msg, e);    break;

                                    default: break;
                                }
                            } else {
                                switch (vec_read<FSCODE>(msg->data)) {

                                    case FSCODE::GIVE:      local_give(msg, e);      break;
                                    case FSCODE::SETTLE:    local_settle(msg, e);    break;
                                    case FSCODE::ASK:       local_ask(msg, e);       break;
                                    case FSCODE::REVOKE:    local_revoke(msg, e);    break;

                                    default: break;
                                }
                            }
                            if (e.is_err()) handle_err(e, (Actors)msg->from);

                        } else {
                            // Just doing this to increment drop count
                            chans_.out_.reserve(Priority::Work);
                        }
                    }
                    

                    if (!msg->is_wiped) msg_wipe(msg);

                    if (msg->from == act_code(Actors::FS)) {
                        buffers_.put(msg->data);
                    } else {
                        // Return message
                        Msg* r_m = chans_.out_.reserve(Priority::Control);
                        if (r_m) {
                            *r_m  = *msg;
                            chans_.out_.commit(Priority::Control);
                        } else {
                            free(msg->data->b);
                            delete msg->data;
                        }
                    }
                    chans_.in_.pop();

                    if (++k > 32) break;

                }
            } else {
                // OTHER FD
            }
        }

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


        const int MAX_OUTS_PER_ROUND = 8;
        for (int i = 0; i < MAX_OUTS_PER_ROUND; i++) {
            Session& s = outbound_.front();
            Msg* m = chans_.out_.reserve(Priority::Work);
            m->data = buffers_.grab(BufferSize::SU);
            m->too = s.actor;

            vec_write(m->data, FSCODE::REWARD);
            vec_write(m->data, s.id);

            uint64_t offset = s.chunk_size * s.chunk_idx;
            if (fseek(s.file->f, offset,  SEEK_SET) < 0) {
                handle_err({
                    .r = n,
                    .code = E_INTERNAL,
                    .key = s.voucher.to,
                    .msg = "Failed to seek in file to outbound session."
                }, (Actors)s.actor);

                if (s.file->ref_count == 1) {
                    fclose(s.file->f);
                    open_files_.erase(s.file->hash);
                }
                outbound_.pop_front();
                continue;
            }


            uint64_t next_chunk_size = std::min(s.chunk_size, s.file->size - offset);

            size_t remaining = next_chunk_size;
            unsigned char* c = m->data->c;
            int n;
            while (n > 0 && remaining > 0) {
                n = fread(c, 1, remaining, s.file->f);
                if (n > 0) {
                    remaining -= n;
                    c += n;
                }
            }

            if (remaining == 0) {
                s.chunk_idx++;

                if (s.chunk_idx * s.chunk_size >= s.file->size) {
                    if (s.file->ref_count == 1) {
                        fclose(s.file->f);
                        open_files_.erase(s.file->hash);
                    }
                    outbound_.pop_front();
                }

                chans_.out_.commit(Priority::Work);

            } else if (n < 0) {
                handle_err({
                    .r = n,
                    .code = E_INTERNAL,
                    .key = s.voucher.to,
                    .msg = "Failed to read chunk from file to outbound session."
                }, (Actors)s.actor);

                if (s.file->ref_count == 1) {
                    fclose(s.file->f);
                    open_files_.erase(s.file->hash);
                }
                outbound_.pop_front();
                continue;
            }


        }
    }
}
