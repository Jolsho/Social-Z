#include <cassert>
#include <sodium/crypto_auth.h>
#include <sys/epoll.h>
#include <thread>
#include <sodium.h>

#include "net/netman.h"
#include "social/social.h"
#include "fs/fs.h"

bool dispatch_packet_event(
    epoll_event& event,
    msging::ChannelPair& net,
    msging::ChannelPair& social,
    msging::ChannelPair& filesys
) {
    msging::ChannelPair* chan;

    int fd = event.data.fd;
    if (fd == net.from.get_event_fd()) {
        net.from.clear_event(); 
        chan = &net;
    } else if (fd == social.from.get_event_fd()) {
        social.from.clear_event(); 
        chan = &social;
    } else if (fd == filesys.from.get_event_fd()) {
        filesys.from.clear_event(); 
        chan = &filesys;
    } else {
        return false;
    }

    while (auto* pkt = chan->from.pop()) {
        if (pkt->too == net::Actors::Networker) {
            net.to.push(pkt);
        } else if (pkt->too == net::Actors::Social) {
            social.to.push(pkt);
        } else if (pkt->too == net::Actors::FileSys) {
            filesys.to.push(pkt);
        } else {
            net::wipe_packet(pkt);
            chan->to.push(pkt);
        }
    }
    return true;
}


int main() {
    assert(sodium_init() != -1); 

    int main_epoll_fd = epoll_create1(0);

    msging::ChannelPair net = msging::new_channel(1024);
    msging::register_queue(main_epoll_fd, net.from);
    std::thread net_thread([&]{
        netman::NetworkManager net_mgr(net);
        net_mgr.poll_loop(); 
    });

    msging::ChannelPair social = msging::new_channel(1024);
    register_queue(main_epoll_fd, social.from);
    std::thread social_thread([&]{
        socializer::Socializer social_mgr(social);
        social_mgr.poll_loop(); 
    });

    msging::ChannelPair filesys = msging::new_channel(1024);
    register_queue(main_epoll_fd, filesys.from);
    std::thread filesys_thread([&]{
        fs::FileManager file_mgr(filesys);
        file_mgr.poll_loop(); 
    });

    while (true) {
        epoll_event events[16];
        int n = epoll_wait(main_epoll_fd, events, 16, -1);

        for (int i = 0; i < n; ++i) {
            if (dispatch_packet_event(events[i], net, social, filesys)) 
                continue;

        }
    }

    net_thread.join();
    social_thread.join();
}
