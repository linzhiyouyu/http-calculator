#ifndef __SOCK_HPP__
#define __SOCK_HPP__
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<unistd.h>
#include"log.hpp"
#include<fcntl.h>
#define DEFAULT_SOCK_VALUE -1
#define DEFAULT_BACKLOG 5
class sock
{
public:
    sock(): sockfd_(DEFAULT_SOCK_VALUE), is_closed_(true) {}
    sock(int sockfd) : sockfd_(sockfd), is_closed_(false) {}
    ~sock() {
        if(!is_closed_) {
            sock_close();
        }
    }
public:
    int fd() const { return sockfd_; }
    bool sock_create() {
        sockfd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        is_closed_ = false;
        if(sockfd_ < 0) {
            ERROR_LOG("套接字创建失败");
            return false;
        }
        return true;
    }
    bool sock_bind(const std::string& ip, uint16_t port) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        int ret = bind(sockfd_, (struct sockaddr*)&addr, sizeof addr);
        if(ret < 0) {
            ERROR_LOG("套接字绑定失败");
            return false;
        }
        return true;
    }
    bool sock_listen(int backlog = DEFAULT_BACKLOG) {
        int ret = listen(sockfd_, backlog);
        if(ret < 0) {
            ERROR_LOG("套接字监听失败");
            return false;
        }
        return true;
    }
    int sock_accept() {
        struct sockaddr_in peer;
        socklen_t len = sizeof peer;
        int channel_fd = accept(sockfd_, (struct sockaddr*)&peer, &len);
        if(channel_fd < 0) {
            ERROR_LOG("套接字接收失败");
            return -1;
        }
        return channel_fd;
    }
    bool sock_connect(const std::string& ip, uint16_t port) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        int ret = connect(sockfd_, (struct sockaddr*)&addr, sizeof addr);
        if(ret < 0) {
            ERROR_LOG("套接字连接失败");
            return false;
        }
        return true;
    }
    void sock_close() {
        if(sockfd_ != DEFAULT_SOCK_VALUE) {
            close(sockfd_);
            is_closed_ = true;
            sockfd_ = DEFAULT_SOCK_VALUE;
        }
    }

    ssize_t sock_recv(void* buf, size_t len, int flag = 0) {
        ssize_t ret = recv(sockfd_, buf, len, flag);
        if(ret <= 0) {
            if(errno == EAGAIN || errno == EINTR) {
                return 0; /*本次没有接收到数据*/
            }
            ERROR_LOG("套接字接收失败");
            return -1;
        }
        return ret;
    }
    ssize_t sock_send(void* buf, size_t len, int flag = 0) {
        ssize_t ret = send(sockfd_, buf, len, flag);
        if(ret < 0) {
            if(errno == EAGAIN || errno == EINTR) {
                return 0; /*本次没有发送数据*/
            }
            ERROR_LOG("套接字发送失败");
            return -1;
        }
        return ret;
    }
    ssize_t nonblock_recv(void* buf, size_t len) {
        return sock_recv(buf, len, MSG_DONTWAIT);
    }
    ssize_t nonblock_send(void* buf, size_t len) {
        return sock_send(buf, len, MSG_DONTWAIT);
    }
    bool create_server(uint16_t port, const std::string& ip = "0.0.0.0", bool flag = false) {
        if(!sock_create()) return false;
        if(flag == true) set_nonblock();
        if(!sock_bind(ip, port)) return false;
        if(!sock_listen()) return false;
        set_reuseaddr();
        return true;
    }
    bool create_client(uint16_t port, const std::string& ip) {
        sock_create();
        if(!sock_connect(ip, port)) return false;
        return true;
    }
    void set_nonblock() {
        int flag = fcntl(sockfd_, F_GETFL, 0);
        flag |= O_NONBLOCK;
        fcntl(sockfd_, F_SETFL, flag);
    }
    void set_reuseaddr() {
        int opt = 1;
        setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    }

private:
    int sockfd_;
    bool is_closed_;
};



#endif // __SOCK_HPP__