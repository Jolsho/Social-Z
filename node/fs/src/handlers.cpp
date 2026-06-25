#include "manager.h"
#include "api/paths.h"
#include "utils/vec.h"

void FS::handle_internal_msgs() {
    while (Msg* msg = consume_msg(in_msgs_)) {
        if (!msg->is_wiped && msg->data && msg->data->len > sizeof(FS_PATH)) {

            // If no space just silently drop
            Error e { 
                .id = msg->id 
            };

            if (msg->from == ACTOR_P2P) {
                switch (vec_read<FS_PATH>(msg->data)) {
                    case FS_VOUCHER:   voucher(msg, e);   break;
                    case FS_REDEEM:    redeem(msg, e);    break;
                    case FS_REWARD:    reward(msg, e);    break;

                    case FS_GIVE:      give(msg, e);      break;
                    case FS_SETTLE:    settle(msg, e);    break;
                    case FS_ASK:       ask(msg, e);       break;
                    case FS_REVOKE:    revoke(msg, e);    break;

                    default: break;
                }
            } else {
                switch (vec_read<FS_PATH>(msg->data)) {

                    case FS_GIVE:      local_give(msg, e);      break;
                    case FS_SETTLE:    local_settle(msg, e);    break;
                    case FS_ASK:       local_ask(msg, e);       break;
                    case FS_REVOKE:    local_revoke(msg, e);    break;

                    default: break;
                }
            }
            if (e.is_err()) handle_err(e, (Actors)msg->from);
        }
        

        if (!msg->is_wiped) msg_wipe(msg);

        if (msg->from == ACTOR_FS) {
            buffers_.put(msg->data);
        } else {
            // Return message
            Msg* m = consume_msg(free_out_msgs_);
            *m = *msg;
            m->priority = PRIORITY_WORK;
        }

        update_actor(chans_, &in_msgs_->consumed_, &free_out_msgs_->consumed_);
    }
}

void FS::handle_outbound() {
    const int MAX_OUTS_PER_ROUND = 8;
    for (int i = 0; i < MAX_OUTS_PER_ROUND; i++) {
        Session& s = outbound_.front();
        Msg* m = consume_msg(free_out_msgs_);
        m->priority = PRIORITY_WORK;
        m->data = buffers_.grab(BufferSize::SU);
        m->too = s.actor;

        vec_write(m->data, FS_REWARD);
        vec_write(m->data, s.id);

        uint64_t offset = s.chunk_size * s.chunk_idx;
        if (fseek(s.file->f, offset,  SEEK_SET) < 0) {
            handle_err({
                .code = E_INTERNAL,
                .key = s.voucher.to,
                .msg = "Failed to seek in file to outbound session."
            }, (Actors)s.actor);

            if (s.file->ref_count == 1) {
                fclose(s.file->f);
                open_files_.erase(s.file->hash);
            }
            outbound_.pop_front();
            unconsume_msg(free_out_msgs_);

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
            unconsume_msg(free_out_msgs_);
            continue;
        }
    }
}
