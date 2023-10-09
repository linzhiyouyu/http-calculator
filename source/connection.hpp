#ifndef __CONNECTION_HPP__
#define __CONNECTION_HPP__
#include"log.hpp"
#include"buffer.hpp"
#include"channel.hpp"
#include"eventloop.hpp"
#include<unordered_map>
#include"sock.hpp"
#include<functional>
#include"any.hpp"

enum ConnStatus{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING
};

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using shared_connection = std::shared_ptr<Connection>;
    using connected_callback = std::function<void(const shared_connection&)>;
    using message_callback = std::function<void(const shared_connection&, Buffer*)>;
    using closed_callback = std::function<void(const shared_connection&)>;
    using any_event_callback = std::function<void(const shared_connection&)>;
public:
    Connection(EventLoop* loop, int conn_id, int sockfd)
        : conn_id_(conn_id), sockfd_(sockfd), _enable_inactive_release_flag(false),
        loop_(loop), status_(CONNECTING), sock_(sockfd), channel_(sockfd_, loop_) 
    {
        channel_.set_close_callback(std::bind(&Connection::handle_close, this));
        channel_.set_any_event_callback(std::bind(&Connection::handle_any_event, this));
        channel_.set_read_callback(std::bind(&Connection::handle_read, this));
        channel_.set_write_callback(std::bind(&Connection::handle_write, this));
        channel_.set_error_callback(std::bind(&Connection::handle_error, this));
    }
    ~Connection()
    {
        DEBUG_LOG("connection release: %p", this);
    }
public:
    void conn_send(const char* data, size_t len) {
        Buffer buf;
        buf.write(data, len);
        buf.move_write_offset(len);
        loop_->run_in_loop(std::bind(&Connection::conn_send_in_loop, this, buf));
    }
    void conn_shutdown() {
        loop_->run_in_loop(std::bind(&Connection::conn_shutdown_in_loop, this));
    }
    void conn_enable_inactive_release(int sec) {
        loop_->run_in_loop(std::bind(&Connection::conn_enable_inactive_release_in_loop, this, sec));
    }
    void conn_cancel_inactive_release() {
        loop_->run_in_loop(std::bind(&Connection::conn_cancel_inactive_release_in_loop, this));
    }
    void conn_protocol_upgrade(const Any& protocol, const connected_callback &conn, 
        const message_callback &msg, 
        const closed_callback &closed, 
        const any_event_callback &any_event)
    {
        loop_->assert_in_loop(); //断言在IO线程中，不能由其他线程调用
        loop_->run_in_loop(std::bind(&Connection::conn_protocol_upgrade_in_loop, this, protocol, conn, msg, closed, any_event));
    }
    void conn_establish() {
        loop_->run_in_loop(std::bind(&Connection::establish_in_loop, this));
    }
public:
/*get方法*/
    int fd() const { return sockfd_; }
    uint64_t conn_id() const { return conn_id_; }
    ConnStatus status() const { return status_; }
    bool is_connected() const { return status_ == CONNECTED; }
    Any* get_protocol_context() { return &context_; }
/*set方法*/
public:
    void set_connected_callback(const connected_callback& cb) { connected_callback_ = cb; }
    void set_message_callback(const message_callback& cb) { message_callback_ = cb; }
    void set_closed_callback(const closed_callback& cb) { closed_callback_ = cb; }
    void set_any_event_callback(const any_event_callback& cb) { any_event_callback_ = cb; }
    void set_protocol_context(const Any& context) { context_ = context; }
    void set_server_closed_callback(const closed_callback& cb) { server_closed_callback_ = cb; }
/*五个回调*/
private:
    void handle_read() {
        std::cout << "connection handle_read" << std::endl;
        char buf[65536];
        ssize_t ret = sock_.nonblock_recv(buf, sizeof buf);
        if(ret < 0) {
            //出错了不能直接关闭连接
            return conn_shutdown_in_loop();
        }
        in_buffer_.write(buf, ret);
        in_buffer_.move_write_offset(ret);
        if(in_buffer_.readable_size() > 0) {
            std::cout << "message_callback_" << std::endl;      
            return message_callback_(shared_from_this(), &in_buffer_);
        }
    }
    void handle_write() {
        ssize_t ret = sock_.nonblock_send(out_buffer_.read_start_position(), out_buffer_.readable_size());
        if(ret < 0) {
            if(in_buffer_.readable_size() > 0) {
                message_callback_(shared_from_this(), &in_buffer_);
            }
            return release_in_loop();
        }
        out_buffer_.move_read_offset(ret);
        if(out_buffer_.readable_size() == 0) {
            channel_.disable_write();
            if(status_ == DISCONNECTING) {
                return conn_shutdown_in_loop();
            }
        }
    }
    void handle_close() {
        if(in_buffer_.readable_size() > 0) {
            message_callback_(shared_from_this(), &in_buffer_);
        }
        return release_in_loop();
    }
    void handle_error() {
        if(in_buffer_.readable_size() > 0) {
            message_callback_(shared_from_this(), &in_buffer_);
        }
        return release_in_loop();
    }
    void handle_any_event() {
        //刷新活跃度，调用上层的任意事件回调
        if(_enable_inactive_release_flag == true) {
            loop_->timer_refresh(conn_id_);
        }
        if(any_event_callback_) {
            any_event_callback_(shared_from_this());
        }
    }
private:
    void conn_send_in_loop(Buffer buf) {
        if(status_ == DISCONNECTED) {
            return;
        }
        out_buffer_.write_buffer_and_push(buf);
        if(channel_.writable() == false) {
            channel_.enable_write();
        }
    }
    void conn_shutdown_in_loop() {
        status_ = DISCONNECTING; //设置连接为半关闭状态
        if(in_buffer_.readable_size() > 0) {
            message_callback_(shared_from_this(), &in_buffer_);
        }
        if(out_buffer_.readable_size() > 0) {
            if(channel_.writable() == false) {
                channel_.enable_write();
            }
        }
        if(out_buffer_.readable_size() == 0) {
            return release_in_loop();
        }
    }
    void conn_enable_inactive_release_in_loop(int sec) {
        _enable_inactive_release_flag = true;
        //如果当前定时销毁任务已经存在，刷新延迟一下即可，如果不存在则新增
        if(loop_->has_timer(conn_id_)) {
            loop_->timer_refresh(conn_id_);
        } else {
            loop_->timer_add(conn_id_, sec, std::bind(&Connection::release_in_loop, this));
        }
    }
    void conn_cancel_inactive_release_in_loop() {
        _enable_inactive_release_flag = false;
        if(loop_->has_timer(conn_id_)) {
            loop_->timer_cancel(conn_id_);
        }
    }
    void release_in_loop() {
        status_ = DISCONNECTED;
        channel_.remove();
        sock_.sock_close();
        if(loop_->has_timer(conn_id_)) {
            loop_->timer_cancel(conn_id_);
        }
        if(closed_callback_) {
            closed_callback_(shared_from_this());
        }
        if(server_closed_callback_) {
            server_closed_callback_(shared_from_this());
        }
    }
    void establish_in_loop() {
        assert(status_ == CONNECTING);
        status_ = CONNECTED;
        channel_.enable_read();
        if(connected_callback_) {
            connected_callback_(shared_from_this());
        }
    }
    void conn_protocol_upgrade_in_loop(const Any& protocol, const connected_callback &conn, 
        const message_callback &msg, 
        const closed_callback &closed, 
        const any_event_callback &any_event)
    {
        context_ = protocol;
        connected_callback_ = conn;
        message_callback_ = msg;
        closed_callback_ = closed;
        any_event_callback_ = any_event;
    }
private:
    uint64_t conn_id_;
    //uint64_t timer_id_;
    int sockfd_;
    ConnStatus status_;
    bool _enable_inactive_release_flag;
    sock sock_;
    Buffer in_buffer_;
    Buffer out_buffer_;
    EventLoop* loop_;
    Channel channel_;
    Any context_;
    connected_callback connected_callback_;
    message_callback message_callback_;
    closed_callback closed_callback_;
    any_event_callback any_event_callback_;
    closed_callback server_closed_callback_;
};

#endif // __CONNECTION_HPP__