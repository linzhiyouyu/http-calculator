#include"httpserver.hpp"
#include<cassert>


int main()
{
    sock cli_sock;
    cli_sock.create_client(8085, "127.0.0.1");
    std::string req = "GET /hello HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    while(1) {
        assert(cli_sock.sock_send(&req[0], req.size()) != -1);
        char buffer[1024] = {0};
        assert(cli_sock.sock_recv(buffer, sizeof(buffer)));
        DEBUG_LOG("recv data: %s", buffer);
        sleep(15);
    }
    cli_sock.sock_close();
    return 0;
}