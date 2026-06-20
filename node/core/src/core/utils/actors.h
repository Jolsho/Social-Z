#pragma once
#include "bindings.h"
#include <vector>

class PollableActor {
public:
    Msg*   out      = nullptr;
    size_t out_n    = 0;
    size_t out_size = 0;

    Msg*   in       = nullptr;
    size_t in_n     = 0;
    size_t in_size  = 0;

    inline Msg* use_out_msg(Priority p) {
        if (out_n == out_size) return NULL;
        Msg* m = out + (out_size - out_n); // From back
        out_n++;
        m->priority = p;
        return m;
    }

    inline void put_back_last_out_msg() {
        if (0 == out_n) return; 
        out_n--;
        msg_wipe(out + (out_size - out_n));
    }

    inline Msg* process_in_msg() {
        if (in_n == in_size) return NULL;
        Msg* m = in + in_n++;
        return m;
    }
};


class Ids {
    ConnID                              next_id_;
    std::vector<ConnID>                 free_ids_;

public:
    Ids(size_t cap);
    inline void put_back_id(ConnID id) {
        free_ids_.push_back(id);
    }
    
    inline void get_next_id(ConnID* id) {
        if (!free_ids_.empty()) {
            *id = free_ids_.back();
            free_ids_.pop_back();
        } else {
            *id = next_id_++;
        }
    }
};
