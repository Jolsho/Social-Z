#pragma once
#include "utils/queue.h"
#include <memory>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <array>

enum class Actors : uint8_t {
    P2P,
    FS,
    SZ,
    DB,
    BC,
    LOG,

    COUNT,
    NONE,
};
constexpr uint8_t act_code(Actors p) { return static_cast<uint8_t>(p); }

enum class Priority : uint8_t {
    Critical = 0,   // shutdown, abort, kill-switch
    Control,        // commands and state transitions
    Work,           // normal processing
    Telemetry,      // logs, metrics, traces
    Count
};

constexpr Priority priority(size_t p) { return static_cast<Priority>(p); }
constexpr size_t idx(Priority p) { return static_cast<size_t>(p); }

template<typename T>
using ChanArray = std::array<T, idx(Priority::Count)>;


struct QueueStats {
    uint64_t    accepted;
    uint64_t    dropped;
    float       drop_rate;

    uint64_t    delta_accepted;
    uint64_t    delta_dropped;
    float       delta_drop_rate;
};

struct ChanSetStats {
    QueueStats  critical;
    QueueStats  control;
    QueueStats  work;
    QueueStats  telemetry;
};

struct ChanStatsPair {
    time_t          timestamp;
    ChanSetStats    in;
    ChanSetStats    out;
};

class ChanSet {
    const ChanArray<int>  budgets_;

    ChanArray<SPSCQueue> qs_;

    size_t          current_;
    ChanArray<int>  counts_;

    ChanArray<uint64_t> accepted_{};
    ChanArray<uint64_t> dropped_{};
    ChanArray<uint64_t> dropped_last_log_{};
    ChanArray<uint64_t> accepted_last_log_{};

    QueueStats stats(Priority p);

public:

    ChanSet(size_t cap, Actors parent, const ChanArray<int> budgets) : 
        qs_{
            SPSCQueue(cap, idx(Priority::Critical), act_code(parent)),
            SPSCQueue(cap, idx(Priority::Control), act_code(parent)),
            SPSCQueue(cap, idx(Priority::Work), act_code(parent)),
            SPSCQueue(cap, idx(Priority::Telemetry), act_code(parent))
        },
        budgets_(budgets)
    {}
    inline Msg* reserve(Priority p) {
        auto i = idx(p);

        auto* msg = qs_[i].reserve();

        if (!msg) ++dropped_[i];

        return msg;
    }

    inline void commit (Priority p) {
        ++accepted_[idx(p)];
        qs_[idx(p)].commit();
    }

    inline bool has_space(Priority p) {
        return qs_[idx(p)].has_space();
    }

    bool front(Msg* m);
    void pop(size_t p = idx(Priority::Count));

    inline ChanSetStats snapshot() {
        return {
            .critical   = stats(Priority::Critical),
            .control    = stats(Priority::Control),
            .work       = stats(Priority::Work),
            .telemetry  = stats(Priority::Telemetry)
        };
    }
};


class ActorChannel {

private:
    int    event_fd_;

    static constexpr ChanArray<int>  DEFAULT_BUDGETS { 32, 16, 8, 2 };

    time_t next_snapshot = time(nullptr) + 20;

public:
    ChanSet out_;
    ChanSet in_;

    ActorChannel(size_t cap, Actors parent, ChanArray<int> budgets = DEFAULT_BUDGETS) : 
        out_(cap, parent, budgets), 
        in_(cap, parent, budgets) 
    {
        event_fd_ = eventfd(0, EFD_NONBLOCK);
    }

    ~ActorChannel() {
        close(event_fd_);
    }

    void clear_event() {
        uint64_t val;
        read(event_fd_, &val, sizeof(val));
    }

    inline int get_event_fd() const { 
        return event_fd_; 
    }

    std::unique_ptr<ChanStatsPair> poll_telemetry() {
        time_t now = time(nullptr);
        if (now < next_snapshot) {
            next_snapshot = now + 10;

            return std::make_unique<ChanStatsPair>(
                ChanStatsPair{ 
                    .timestamp = now,
                    .in = in_.snapshot(),
                    .out = out_.snapshot(),
                }
            );
        }
        return nullptr;
    }
};

inline void register_queue(
    int epoll_fd, ActorChannel& q
) {
    epoll_event ev{
        .events = EPOLLIN, 
        .data = {
            .fd = q.get_event_fd()
        },
    };
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, q.get_event_fd(), &ev);
}
