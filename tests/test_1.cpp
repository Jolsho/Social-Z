#include "crypto.h"
#include "init.h"
#include <cstdio>
#include <gtest/gtest.h>
#include <sodium/core.h>
#include <sys/epoll.h>

class P2PTest : public ::testing::Test {
protected:

    int main_epoll_fd1;
    std::array<ActorChannels, Actors::COUNT> actors1 {};

    int main_epoll_fd2;
    std::array<ActorChannels, Actors::COUNT> actors2 {};

    void SetUp() override {
        assert(sodium_init() != -1); 
        main_epoll_fd1 = epoll_create1(0);
        main_epoll_fd2 = epoll_create1(0);
    }

    void TearDown() override {
        // runs after each test
        printf("TEAR DOWN\n");
    }
};

TEST_F(P2PTest, Basic) {

    std::thread p2p_thread1 = start_p2p(main_epoll_fd1, actors1[Actors::PEERNET], {
        .msgs_cap   = 8,
        .pkts_cap   = 8,
        .port       = 8081,
        .ip         = "127.0.0.1"
    });
    std::thread main_thread1 = main_loop(main_epoll_fd1, actors1, [](msg::Msg* m){
        if (m->from == Actors::PEERNET) {
            printf("FROM 1 PEERNET\n");
        } else if (m->from == Actors::FILESYS) {
            printf("FROM 1 FS\n");
        }
    });


    KeyPair key_2 {};
    EXPECT_EQ(new_keypair(key_2), 0);

    std::thread p2p_thread2 = start_p2p(main_epoll_fd2, actors2[Actors::PEERNET], {
        .msgs_cap   = 8,
        .pkts_cap   = 8,
        .port       = 8082,
        .ip         = "127.0.0.1",
        .key        = key_2.priv
    });

    std::thread main_thread2 = main_loop(main_epoll_fd2, actors2, [](msg::Msg* m){
        if (m->from == Actors::PEERNET) {
            printf("FROM 2 PEERNET\n");
        } else if (m->from == Actors::FILESYS) {
            printf("FROM 2 FS\n");
        }
    });

    // ACTOR1 connect -> ACTOR2
    msg::Msg* m = new msg::Msg{ Actors::NONE };
    std::string key_2_str = key_to_str(key_2.pub);
    EXPECT_EQ(msg::p2p_new_conn(m, "127.0.0.1", 8082, key_2_str), 0);
    actors1[Actors::NONE].from.push(m);



    main_thread1.join();
    p2p_thread1.join();

    main_thread2.join();
    p2p_thread2.join();
}
