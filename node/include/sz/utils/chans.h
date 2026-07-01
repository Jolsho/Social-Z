#pragma once
#include "sz/api/actor.h"
#include "sz/api/msgT.h"
#include "sz/utils/queue.h"
#include <array>

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
            const ChannelSizes* sizes,
            const ChannelSizes* budgets
        ) :
        q_ {
            SPSCQueue<size_t>{sizes->critical, parent},
            SPSCQueue<size_t>{sizes->control, parent},
            SPSCQueue<size_t>{sizes->work, parent},
            SPSCQueue<size_t>{sizes->telemetry, parent},
        },
        free_ {
            SPSCQueue<size_t>{msg_cap, parent},
        },
        in_use_ {
            SPSCQueue<size_t>{msg_cap, parent},
        },
        msgs_(msg_cap)
    { 
        budgets_[PRIORITY_CRIT] = budgets->critical;
        budgets_[PRIORITY_CONT] = budgets->control;
        budgets_[PRIORITY_WORK] = budgets->work;
        budgets_[PRIORITY_TELE] = budgets->telemetry;
    }

    inline bool has_space(Priority p) { 
        return q_[p].has_space(); 
    }

    inline void change_budget(size_t idx, size_t bud) {
        budgets_[idx] = bud;
    }

    void poll(MsgBuffer* msgs);
    void get_free_msgs(MsgBuffer* msgs);

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

