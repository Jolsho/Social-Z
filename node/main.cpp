/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "sz_node/actor.h"
#include "sz_node/sz.h"
#include "sz_node/db.h"
#include "sz_node/fs.h"
#include "sz_node/log.h"
#include "sz_node/p2p.h"

int main() {
    SZT* szt = new_sz();

    ActorConfig conf = default_actor_config(ACTOR_DB, 256);

    DBConfig db_conf = {};
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
