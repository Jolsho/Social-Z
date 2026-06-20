#pragma once
#include "bindings.h"
#include "utils/queue.h"
#include <span>

template <typename T> using ChanArray = std::array<T, PRIORITY_COUNT>;

class Channel {

    ChanArray<size_t>               budgets_;
    std::vector<Msg>                msgs_;

    ChanArray<SPSCQueue<size_t>>    q_;
    SPSCQueue<size_t>               free_;
    SPSCQueue<size_t>               in_use_;

    size_t              current_;
    ChanArray<int>      counts_;

    ChanArray<uint64_t> accepted_           {};
    ChanArray<uint64_t> dropped_            {};
    ChanArray<uint64_t> dropped_last_log_   {};
    ChanArray<uint64_t> accepted_last_log_  {};

    QueueStats stats(Priority p);

public:
    // CHAN_SIZE => sizes * (sizeof(Msg) + sizeof(Msg*) * chan(q_,free_,in_use_))
    // 1024 * (32 + 8*3) => 58kb
   
    // (actors * (58k + budgets) * actor(in_, out_))
    // (9 * (58k + 512) * 2) => 1mb

    Channel(size_t msg_cap, 
            Actors parent, 
            const ChanArray<size_t> sizes,
            const ChanArray<size_t> budgets
        ) :
        budgets_(budgets),
        q_ {
            SPSCQueue<size_t>{sizes[PRIORITY_CRIT], parent},
            SPSCQueue<size_t>{sizes[PRIORITY_CONT], parent},
            SPSCQueue<size_t>{sizes[PRIORITY_WORK], parent},
            SPSCQueue<size_t>{sizes[PRIORITY_TELE], parent},
        },
        free_ {
            SPSCQueue<size_t>{msg_cap, parent},
        },
        in_use_ {
            SPSCQueue<size_t>{msg_cap, parent},
        },
        msgs_(msg_cap)
    { 

    }

    inline bool has_space(Priority p) { 
        return q_[p].has_space(); 
    }

    inline void change_budget(size_t idx, size_t bud) {
        budgets_[idx] = bud;
    }

    // TODO -- these need to take in MsgBuffers
    // you cant just append to spans of them
    size_t poll(std::span<Msg*>&& msgs);
    size_t get_free_msgs(std::span<Msg*>&& msgs);

    void free_msgs(size_t size);
    void use_free_msgs(size_t size);


    inline ChanSetStats snapshot() {
        return {
            .critical = stats(PRIORITY_CRIT),
            .control = stats(PRIORITY_CONT),
            .work = stats(PRIORITY_WORK),
            .telemetry = stats(PRIORITY_TELE)
        };
    }
};

