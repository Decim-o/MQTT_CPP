#ifndef NETWORK_HPP
#ifndef NETWORK_HPP


#include <cstddef>
#include <cstdint>
#include <string_view>

#include <sys/types.h>

namespace Network{

    enum class SocketFamily {
    Unix,
    Inet
};

int set_nonblocking(int fd);
int set_tcp_nodelay(int fd);
int create_and_bind(std::string_view host, 
                    std::string_view port, 
                    SocketFamily family);

int make_listen(std::string_view host,
                std::string_view port,
                SocketFamily family,
                int backlog);

int accept_connection(int serversock, SocketFamily family);

ssize_t send_bytes(int fd, 
                   const std::uint8_t* buffer, 
                   std::size_t length);

ssize_t recv_bytes(int fd,
                   std::uint8_t* buffer,
                   std::size_t buffer_size);
} // namespace network

#endif
