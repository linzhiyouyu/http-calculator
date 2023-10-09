#ifndef __POLLER_HPP__
#define __POLLER_HPP__
#include<unordered_map>
#include<sys/epoll.h>
#include"channel.hpp"
#include"log.hpp"
#include<unistd.h>
#include<cstring>
#define MAX_EVENTS 1024
class Poller
{
public:
    Poller() {
        epfd_ = epoll_create(1);
        if(epfd_ < 0) {
            ERROR_LOG("epoll_create error");
            abort();
        }
    }
    ~Poller() {
        close(epfd_);
    }
public:
    void update_event(Channel* channel)
    {
        bool ret = has_channel(channel);
        if(ret == false) {
            _channels.emplace(channel->fd(), channel);
            update(channel, EPOLL_CTL_ADD);
        }
        else {
            update(channel, EPOLL_CTL_MOD);
        }
    }
    void remove_event(Channel* channel)
    {
        auto it = _channels.find(channel->fd());
        if(it == _channels.end()) {
            abort();
        }
        _channels.erase(it);
        update(channel, EPOLL_CTL_DEL);
    }
    void poll(std::vector<Channel*>* actives)
    {
        int nfds = epoll_wait(epfd_, _evs, MAX_EVENTS, -1);
        if(nfds < 0) {
            if(errno == EINTR) {
                return;
            }
            ERROR_LOG("epoll_wait error: %s", strerror(errno));
            abort();
        }
        for(int i = 0; i < nfds; ++i) {
            int fd = _evs[i].data.fd;
            Channel* channel = _channels[fd];
            channel->set_revents(_evs[i].events);
            actives->push_back(channel);
        }
    }
private:
    void update(Channel* channel, int op)
    {
        int fd = channel->fd();
        uint32_t events = channel->events();
        struct epoll_event ev;
        ev.events = events;
        ev.data.fd = fd;
        int ret = epoll_ctl(epfd_, op, fd, &ev);
        if(ret < 0) {
            ERROR_LOG("epoll_ctl error");
            abort();
        }
    }
    bool has_channel(Channel* channel)
    {
        auto it = _channels.find(channel->fd());
        if(it == _channels.end()) {
            return false;
        }
        return true;
    }
private:
    int epfd_;
    struct epoll_event _evs[MAX_EVENTS];
    std::unordered_map<int, Channel*> _channels;
};






#endif // __POLLER_HPP__