#ifndef __TCP_SERVER_HPP__
#define __TCP_SERVER_HPP__

#include"eventloop.hpp"
#include"log.hpp"
#include"loopthread.hpp"
#include"acceptor.hpp"
#include"connection.hpp"
#include<signal.h>
class TcpServer
{
    using connected_callback = std::function<void(const Connection::shared_connection&)>;
    using message_callback = std::function<void(const Connection::shared_connection&, Buffer*)>;
    using closed_callback = std::function<void(const Connection::shared_connection&)>;
    using any_event_callback = std::function<void(const Connection::shared_connection&)>;
    using functor = std::function<void()>;
public:
    TcpServer(int port)
        :port_(port), next_id_(0), enable_inactive_release_flag_(false), 
        acceptor_(&base_loop_, port_), loop_pool_(&base_loop_)
    {}
public:
    void set_thread_count(int count) {
        return loop_pool_.set_thread_count(count);
    }
    void set_connected_callback(const connected_callback& cb) {
        connected_callback_ = cb;
    }
    void set_message_callback(const message_callback& cb) {
        message_callback_ = cb;
    }
    void set_closed_callback(const closed_callback& cb) {
        closed_callback_ = cb;
    }
    void set_any_event_callback(const any_event_callback& cb) {
        any_event_callback_ = cb;
    }
    void enable_inactive_release(uint32_t timeout) {
        enable_inactive_release_flag_ = true;
        timeout_ = timeout;
    }
    void start() {
        acceptor_.set_accept_callback(std::bind(&TcpServer::new_connection, this, std::placeholders::_1));
        acceptor_.listen();
        loop_pool_.create();
        base_loop_.start();
    }
    void run_after(uint32_t timeout, const functor& cb) {
        base_loop_.run_in_loop(std::bind(&TcpServer::run_after_in_loop, this, timeout, cb));
    }
private:
    void new_connection(int fd) {
        next_id_++;
        Connection::shared_connection conn(new Connection(loop_pool_.next_loop(), next_id_, fd));
        conn->set_connected_callback(connected_callback_);
        conn->set_message_callback(message_callback_);
        conn->set_closed_callback(closed_callback_);
        conn->set_any_event_callback(any_event_callback_);
        conn->set_server_closed_callback(std::bind(&TcpServer::remove_connection, this, std::placeholders::_1));
        if(enable_inactive_release_flag_) conn->conn_enable_inactive_release(timeout_);
        conn->conn_establish();
        conn_map_.emplace(next_id_, conn);
    }
    void remove_connection(const Connection::shared_connection& conn) {
        base_loop_.run_in_loop(std::bind(&TcpServer::remove_connection_in_loop, this, conn));
    }
    void remove_connection_in_loop(const Connection::shared_connection& conn) {
        int id = conn->conn_id();
        auto it = conn_map_.find(id);
        if(it != conn_map_.end()) {
            conn_map_.erase(it);
        }
    }
    void run_after_in_loop(uint32_t timeout, const functor& cb)
    {
        next_id_++;
        base_loop_.timer_add(next_id_, timeout, cb);
    }
private:
    int port_;
    uint64_t next_id_;
    EventLoop base_loop_;
    Acceptor acceptor_;
    uint32_t timeout_;
    bool enable_inactive_release_flag_;
    LoopThreadPool loop_pool_;
    std::unordered_map<uint64_t, Connection::shared_connection> conn_map_;
    connected_callback connected_callback_;
    message_callback message_callback_;
    closed_callback closed_callback_;
    any_event_callback any_event_callback_;
};



class NetWork
{
public:
    NetWork() {
        DEBUG_LOG("NetWork init");
        signal(SIGPIPE, SIG_IGN);
    }
};
static NetWork nw;


#endif //__TCP_SERVER_HPP__