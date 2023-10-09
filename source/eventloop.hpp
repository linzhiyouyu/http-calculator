#ifndef __EVENTLOOP_HPP__
#define __EVENTLOOP_HPP__
#include<mutex>
#include<vector>
#include<functional>
#include<sys/eventfd.h>
#include<thread>
#include"poller.hpp"
#include<cassert>

#include"timerwheel.hpp"
#include<iostream>

class EventLoop
{
    using task_func = std::function<void()>;
    using functor = std::function<void()>;
    using task = std::vector<functor>;
public:
    EventLoop()
        :thread_id_(std::this_thread::get_id())
        ,event_fd_(create_event_fd())
        ,event_channel_(new Channel(event_fd_, this))
        ,poller_()
        ,timer_wheel_(this)
    {
        event_channel_->set_read_callback(std::bind(&EventLoop::read_event_of_event_channel, this));
        event_channel_->enable_read();
    }
    ~EventLoop()
    {}
public:
    void start()
    {
        while(1)
        {
            //先进行事件的处理
            std::vector<Channel*> actives;
            poller_.poll(&actives);
            for_each(begin(actives), end(actives), [](Channel* channel){
                channel->handler_event();
            });
            //然后处理定时器等任务
            run_all_task(); 
        }
    }
    bool is_in_loop() const { return thread_id_ == std::this_thread::get_id(); }
    void assert_in_loop() const { assert(is_in_loop()); }
    void run_in_loop(const functor& cb) {
        if(is_in_loop()) return cb();
        else queue_in_loop(cb);
    }
    void queue_in_loop(const functor& cb) {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push_back(cb);
        wake_up_event_fd();
    }
    void update_channel(Channel* channel) {
        poller_.update_event(channel);
    }
    void remove_channel(Channel* channel) {
        poller_.remove_event(channel);
    }
    void timer_add(uint64_t id, uint32_t delay, const task_func& cb) {
        timer_wheel_.timer_add(id, delay, cb);
    }
    void timer_cancel(uint64_t id) {
        timer_wheel_.timer_cancel(id);
    }
    void timer_refresh(uint64_t id) {
        timer_wheel_.timer_refresh(id);
    }
    bool has_timer(uint64_t id) {
        return timer_wheel_.has_timer(id);
    }
private:
    static int create_event_fd()
    {
        int fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if(fd < 0) {
            ERROR_LOG("eventfd创建失败");
            abort();
        }
        return fd;
    }
    void read_event_of_event_channel()
    {
        uint64_t res = 0;
        int ret = read(event_fd_, &res, sizeof res);
        if(ret < 0) {
            if(errno == EINTR || errno == EAGAIN) return;
            ERROR_LOG("eventfd读取失败");
            abort();
        }
        return;
    }
    void wake_up_event_fd()
    {
        uint64_t one = 1;
        int ret = write(event_fd_, &one, sizeof one);
        if(ret < 0) {
            if(errno == EINTR || errno == EAGAIN) return;
            ERROR_LOG("eventfd写入失败");
            abort();
        }
        return;
    }
    void run_all_task()
    {
        task task_list;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            task_list.swap(tasks_);
        }
        for_each(begin(task_list), end(task_list), [](const functor& f){
            f();
        });
    }
private:
    std::thread::id thread_id_;
    int event_fd_;
    Poller poller_;
    std::unique_ptr<Channel> event_channel_;
    std::mutex mutex_;
    task tasks_;
    TimerWheel timer_wheel_;
};

void Channel::update()
{
    loop_->update_channel(this);
}

void Channel::remove()
{
    loop_->remove_channel(this);
}


void TimerWheel::timer_add(uint64_t id, uint32_t delay, const task_func& cb) {
    loop_->run_in_loop(std::bind(&TimerWheel::timer_add_in_loop, this, id, delay, cb));
}
void TimerWheel::timer_refresh(uint64_t id) {
    loop_->run_in_loop(std::bind(&TimerWheel::timer_refresh_in_loop, this, id));
}
void TimerWheel::timer_cancel(uint64_t id) {
    loop_->run_in_loop(std::bind(&TimerWheel::timer_cancel_in_loop, this, id));
}

#endif // __EVENTLOOP_HPP__