#ifndef __LOOP_THREAD_HPP__
#define __LOOP_THREAD_HPP__
#include<thread>
#include"eventloop.hpp"
#include<mutex>
#include<condition_variable>
class LoopThread
{
public:
    LoopThread()
        :loop_(nullptr),
        thread_(std::thread(&LoopThread::thread_entry, this))
    {}
public:
    EventLoop* getloop() {
        EventLoop* loop = nullptr;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [&](){return loop_ != nullptr;});
            loop = loop_;
        }
        return loop;
    }
private:    
    void thread_entry()
    {
        EventLoop loop;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            loop_ = &loop;
            cond_.notify_all();
        }
        loop.start();
    }
private:
    std::mutex mutex_;
    std::condition_variable cond_;
    EventLoop* loop_;
    std::thread thread_;
};

class LoopThreadPool
{
public:
    LoopThreadPool(EventLoop* base_loop)
        :thread_count_(0), next_index_(0), base_loop_(base_loop)
    {}
public:
    void set_thread_count(int thread_count) {
        thread_count_ = thread_count;
    }
    void create() {
        if(thread_count_ <= 0) return;
        loop_threads_.resize(thread_count_);
        loops_.resize(thread_count_);
        for(int i = 0; i < thread_count_; ++i) {
            loop_threads_[i] = new LoopThread();
            loops_[i] = loop_threads_[i]->getloop();
        }
    }
    EventLoop* next_loop() {
        if(thread_count_ == 0) return base_loop_;
        EventLoop* loop = loops_[next_index_];
        next_index_ = (next_index_ + 1) % thread_count_;
        return loop;
    }
private:
    int thread_count_;
    int next_index_;
    EventLoop* base_loop_;
    std::vector<LoopThread*> loop_threads_;
    std::vector<EventLoop*> loops_;
};



#endif // __LOOP_THREAD_HPP__