#ifndef __ACCEPTOR_HPP__
#define __ACCEPTOR_HPP__
#include"sock.hpp"
#include"eventloop.hpp"
#include"log.hpp"
#include"connection.hpp"
#include"channel.hpp"
class Acceptor
{
public:
    using accept_callback = std::function<void(int)>;
    Acceptor(EventLoop* loop, int port)
        : loop_(loop), lst_sock_(create_server(port)), lst_channel_(lst_sock_.fd(), loop_)
    {}
public:
    void set_accept_callback(const accept_callback& cb) {
        accept_cb_ = cb;
    }
    void listen() {
        lst_channel_.set_read_callback(std::bind(&Acceptor::handle_read, this));
        lst_channel_.enable_read();
    }
private:
    int create_server(int port) {
        bool ret = lst_sock_.create_server(port);
        assert(ret == true);
        return lst_sock_.fd();
    }
    void handle_read() {
        int fd = lst_sock_.sock_accept();
        if (fd < 0) {
            return;
        }
        if (accept_cb_) {
            accept_cb_(fd);
        }
    }
private:
    sock lst_sock_;
    EventLoop* loop_;
    Channel lst_channel_;
    accept_callback accept_cb_;
};



#endif // __ACCEPTOR_HPP__