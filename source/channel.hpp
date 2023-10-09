#ifndef __CHANNEL_HPP__
#define __CHANNEL_HPP__
#include<iostream>
#include<functional>
#include<sys/epoll.h>
class EventLoop;
class Channel
{
    using event_callback = std::function<void()>;
public:
    Channel(int fd, EventLoop* loop) : fd_(fd), events_(0), revents_(0), loop_(loop) {}
public:
    void set_read_callback(const event_callback& cb) { read_callback_ = cb; }
    void set_write_callback(const event_callback& cb) { write_callback_ = cb; }
    void set_error_callback(const event_callback& cb) { error_callback_ = cb; }
    void set_close_callback(const event_callback& cb) { close_callback_ = cb; }
    void set_any_event_callback(const event_callback& cb) { any_event_callback_ = cb; }
    void set_revents(uint32_t revents) { revents_ = revents; }
    void update();
    void remove();
    uint32_t events() const { return events_; }
    int fd() const { return fd_; }
public:
    bool readable() const { return revents_ & EPOLLIN; }
    bool writable() const { return revents_ & EPOLLOUT; }
    void enable_read() { events_ |= EPOLLIN; update(); }
    void enable_write() { events_ |= EPOLLOUT; update(); }
    void disable_read() { events_ &= ~EPOLLIN; update(); }
    void disable_write() { events_ &= ~EPOLLOUT; update(); }
    void disable_all() { events_ = 0; remove(); }
    void handler_event() {
        if((revents_ & EPOLLIN) || (revents_ & EPOLLRDHUP) || (revents_ & EPOLLPRI)) {
            if(any_event_callback_) { any_event_callback_(); }
            if(read_callback_) { read_callback_(); }
        }
        if(revents_ & EPOLLOUT) {
            if(any_event_callback_) { any_event_callback_(); }
            if(write_callback_) { write_callback_(); }
        }
        else if(revents_ & EPOLLERR) {
            if(any_event_callback_) { any_event_callback_(); }
            if(error_callback_) { error_callback_(); }
        }
        else if(revents_ & EPOLLHUP) {
            if(any_event_callback_) { any_event_callback_(); }
            if(close_callback_) { close_callback_(); }
        }
    }
private:
    int fd_;
    EventLoop* loop_;
    uint32_t events_;
    uint32_t revents_;
    event_callback read_callback_;
    event_callback write_callback_;
    event_callback error_callback_;
    event_callback close_callback_;
    event_callback any_event_callback_;
};

#endif // __CHANNEL_HPP__