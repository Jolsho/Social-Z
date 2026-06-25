#include "api/actor.h"
#include "api/sz.h"
#include "db/db.h"
#include "fs/fs.h"
#include "log/log.h"
#include "p2p/p2p.h"
#include <array>

int main() {
    SZT* szt = new_sz();

    std::array<size_t, PRIORITY_COUNT> in_q_size = {};
    std::array<size_t, PRIORITY_COUNT> in_budgets = {};

    std::array<size_t, PRIORITY_COUNT> out_q_size = {};
    std::array<size_t, PRIORITY_COUNT> out_budgets = {};

    ActorConfig conf {
        .in_q_sizes = in_q_size.data(),
        .in_q_sizes_len = in_q_size.size(),
        .in_budgets = in_budgets.data(),
        .in_budgets_len = in_budgets.size(),

        .out_q_sizes = out_q_size.data(),
        .out_q_sizes_len = out_q_size.size(),
        .out_budgets = out_budgets.data(),
        .out_budgets_len = out_budgets.size(),
    };

    DBConfig db_conf = {};
    conf.id = ACTOR_DB;
    ActorThread* db = start_db(new_actor(szt, &conf), &db_conf);

    FSConfig fs_conf = {};
    conf.id = ACTOR_FS;
    ActorThread* fs = start_fs(new_actor(szt, &conf), &fs_conf);

    conf.id = ACTOR_LEDG;
    Actor* lg = new_actor(szt, &conf);

    conf.id = ACTOR_LOG;
    LogConfig log_conf = {};
    ActorThread* log = start_log(new_actor(szt, &conf), &log_conf);

    conf.id = ACTOR_P2P;
    P2PConfig p2p_conf = {};
    ActorThread* p2p = start_p2p(new_actor(szt, &conf), &p2p_conf);

    int r = run(szt);

    wait_on_actor_thread(db);
    wait_on_actor_thread(fs);
    wait_on_actor_thread(log);
    wait_on_actor_thread(p2p);
}
