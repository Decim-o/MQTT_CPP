#include "network.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <iostream>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>


namespace {


void check(bool condition, const char* message){
    if(!condition){
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

std::uint16_t get_bound_port(int fd){
    sockaddr_in addr {};
    socklen_t length = sizeof(addr);

    int result = getsockname(fd,
                         reinterpret_cast<sockaddr*>(&addr),
                         &length
    );

    check(result == 0, "getsockname failed");
    check(addr.sin_family == AF_INET, "Expected IPv4 socket");

    return ntohs(addr.sin_port);
}

Network::FileD connect_to_listener(int server_fd){
    std::uint16_t port = get_bound_port(server_fd);

    // create a client socket
    Network::FileD client{
        socket(AF_INET, SOCK_STREAM, 0)
    };

    check(client.get() >= 0, "client socket creation failed");

    sockaddr_in serv_addr {};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    check(
        inet_pton(AF_INET,
                  "127.0.0.1",
                  &serv_addr.sin_addr
        ) == 1,
        "inet_pton failure"
    );

    // connect client to our listener
    int result = connect(
            client.get(),
            reinterpret_cast<sockaddr*>(&serv_addr),
            sizeof(serv_addr)
    );

    check(result == 0, "client connect failed");

    return client;
}

void test_make_listen(){
    Network::FileD server = Network::make_listen(
            "127.0.0.1",
            "0",
            Network::SocketFamily::Inet,
            10
    );

    check(server.get() >= 0, "make_listen failed");

    //verify socket is non blocking
    int flags = fcntl(server.get(), F_GETFL, 0);

    check(flags != -1, "fcntl failed");
    check((flags & O_NONBLOCK) != 0, "listening socket is blocking");

    Network::FileD client = connect_to_listener(server.get());

    std::cout << "PASS: make_listen" << "\n";
}

void test_accept_connection(){
    Network::FileD server = Network::make_listen(
        "127.0.0.1",
        "0",
        Network::SocketFamily::Inet,
        10
    );

    check(server.get() >= 0, "failed to create listener");

    Network::FileD client = connect_to_listener(server.get());

    //accept pending connection
    Network::FileD accepted = Network::accept_connection(
        server.get(),
        Network::SocketFamily::Inet
    );

    check(accepted.get() >= 0, "accept_connection failed");

    // verify the accepted socket is non blocking
    int flags = fcntl(accepted.get(), F_GETFL, 0);

    check(flags != -1, "fcntl on accepted socket failed");
    check((flags & O_NONBLOCK) != 0, "accepted socket is blocking");


    //verify tcp_nodelay was enabled
    int nodelay = 0;
    socklen_t len = sizeof(nodelay);

    int result = getsockopt(
            accepted.get(),
            IPPROTO_TCP,
            TCP_NODELAY,
            &nodelay,
            &len
    );

   check(result == 0, "TCP_NODELAY check failed");
   check(nodelay != 0, "TCP_NODELAY is not enabled");

   std::cout << "Pass: accept_connection" << "\n";

}

void test_accept_no_pending_connection(){
    Network::FileD server = Network::make_listen(
        "127.0.0.1",
        "0",
        Network::SocketFamily::Inet,
        10
    );

    check(server.get() >= 0, "failed to create listener");

    // no client has connected
    Network::FileD accepted = Network::accept_connection(
        server.get(),
        Network::SocketFamily::Inet
    );

    check(accepted.get() == -1, "Expected no pending connection");

    std::cout << "Pass: accept with no pending connection" << "\n";
}

} // namespace

int main(){
    test_make_listen();
    test_accept_connection();
    test_accept_no_pending_connection();
    return EXIT_SUCCESS;
}
