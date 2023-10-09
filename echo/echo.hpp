#ifndef __ECHO_HPP__
#define __ECHO_HPP__
#include"../source/tcpserver.hpp"


class EchoServer
{
public:
    EchoServer(int port) : server_(port) {
        server_.set_thread_count(2);
        //server_.enable_inactive_release(10);
        server_.set_message_callback(std::bind(&EchoServer::on_message, this, std::placeholders::_1, std::placeholders::_2));
        server_.set_connected_callback(std::bind(&EchoServer::on_connected, this, std::placeholders::_1));
        server_.set_closed_callback(std::bind(&EchoServer::on_closed, this, std::placeholders::_1));
    }
    void start() {
        server_.start();
    }
private:
    void on_message(const Connection::shared_connection& conn, Buffer* buffer) {
        std::cout << "on_message" << ": " << buffer->read_start_position() << std::endl;
        buffer->move_read_offset(buffer->readable_size());
        std::string str = "hello world";
        conn->conn_send((char*)&str[0], str.size());
        //conn->conn_shutdown();
    }
    void on_connected(const Connection::shared_connection& conn) {
        std::cout << "on_connected" << std::endl;
    }
    void on_closed(const Connection::shared_connection& conn) {
        std::cout << "on_closed" << std::endl;
    }
private:
    TcpServer server_;
};


#endif // __ECHO_HPP__