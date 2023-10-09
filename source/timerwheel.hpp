#ifndef __TIMERWHEEL_HPP__
#define __TIMERWHEEL_HPP__

#include<iostream>
#include<functional>
#include<unordered_map>
#include<vector>
#include<memory>
#include<sys/timerfd.h>
#include"log.hpp"
#include"channel.hpp"
#include<strings.h>
#include<unistd.h>  

using task_func = std::function<void()>;
using release_func = std::function<void()>;
class TimerTask
{
public:
    TimerTask(uint64_t id, uint32_t delay, const task_func& cb)
        :id_(id), timeout_(delay), cb_(cb), canceled_(false)
    {}
    ~TimerTask() 
    {
        if(!canceled_) {
            cb_();
        }
        if(release_) {
            release_();
        }
    }
public:
    void cancel() { canceled_ = true; }
    void set_release(const release_func& release) {
        release_ = release;
    }
    uint32_t timeout() const { return timeout_; }
private:
    uint64_t id_;
    uint32_t timeout_;
    task_func cb_;
    release_func release_;
    bool canceled_;
};

#define DEFAULT_WHEEL_SIZE 60
class TimerWheel
{
    using shared_task = std::shared_ptr<TimerTask>;
    using weak_task = std::weak_ptr<TimerTask>;
public:
    TimerWheel(EventLoop* loop)
        :tick_(0), capacity_(DEFAULT_WHEEL_SIZE), loop_(loop), timerfd_(create_timer_fd())
        ,time_channel_(new Channel(timerfd_, loop_))
    {
        wheel_.resize(capacity_);
        time_channel_->set_read_callback(std::bind(&TimerWheel::on_time, this));
        time_channel_->enable_read();
    }
public:

    
    void timer_add(uint64_t id, uint32_t delay, const task_func& cb);
    void timer_refresh(uint64_t id);
    void timer_cancel(uint64_t id);
    /*有线程安全问题，只能在该线程内部调用，不允许其他线程调用*/
    bool has_timer(uint64_t id) {
        auto it = tasks_.find(id);
        return it != tasks_.end();
    }
    
private:
    void timer_cancel_in_loop(uint64_t id) {
        auto it = tasks_.find(id);
        if(it == tasks_.end()) {
            ERROR_LOG("任务不存在");
            return;
        }
        shared_task task = it->second.lock();
        if(task.get())
            task->cancel();
    }
    void timer_refresh_in_loop(uint64_t id) {
        auto it = tasks_.find(id);
        if(it == tasks_.end()) {
            ERROR_LOG("任务不存在");
            return;
        }
        shared_task task = it->second.lock();
        int delay = task->timeout();
        int position = (tick_ + delay) % capacity_;
        wheel_[position].push_back(task);
    }
    void timer_add_in_loop(uint64_t id, uint32_t delay, const task_func& cb) {
        shared_task task(new TimerTask(id, delay, cb));
        task->set_release(std::bind(&TimerWheel::remove_task, this, id));
        tasks_[id] = weak_task(task);
        int position = (tick_ + delay) % capacity_;
        wheel_[position].push_back(task);
    }
    void time_read_event() {
        uint64_t res = 0;
        int ret = read(timerfd_, &res, sizeof(res));
        if(ret < 0) {
            ERROR_LOG("read timerfd error");
            abort();
        }
    }
    void on_time() {
        time_read_event();
        run_timer_task();
    }
    void run_timer_task() {
        tick_ = (tick_ + 1) % capacity_;
        wheel_[tick_].clear();
    }
    static int create_timer_fd() {
        int timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        if(timerfd < 0)  {
            ERROR_LOG("timerfd_create error");
            abort();
        }
        struct itimerspec howlong;
        bzero(&howlong, sizeof(howlong));
        howlong.it_value.tv_sec = 1;
        howlong.it_value.tv_nsec = 0;
        howlong.it_interval.tv_sec = 1;
        howlong.it_interval.tv_nsec = 0;
        int ret = timerfd_settime(timerfd, 0, &howlong, nullptr);
        if(ret < 0) {
            ERROR_LOG("timerfd_settime error");
            abort();
        }
        return timerfd;
    }
    /*release回调*/
    bool remove_task(uint64_t id) {
        auto it = tasks_.find(id);
        if(it == tasks_.end()) {
            ERROR_LOG("任务不存在");
            return false;
        }
        tasks_.erase(it);
        return true;
    }
private:
    std::vector<std::vector<shared_task>> wheel_;
    int tick_;
    int capacity_;
    std::unordered_map<uint64_t, weak_task> tasks_;
    int timerfd_;
    EventLoop* loop_;
    std::unique_ptr<Channel> time_channel_;
};


#endif // __TIMERWHEEL_HPP__