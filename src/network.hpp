#ifndef NETWORK_HPP
#define NETWORK_HPP 


#include <cstddef>
#include <cstdint>
#include <string_view>

#include <sys/types.h>

namespace Network{

enum class SocketFamily {
    Unix,
    Inet
};

class FileD{
public:
    FileD(int fd = -1) : m_fd(fd) {}

    FileD(const FileD& other) = delete;
    FileD(FileD&& other) noexcept;

    FileD& operator=(const FileD& other) = delete;
    FileD& operator=(FileD&& other) noexcept;

    int get() const {
        return m_fd;
    }

    ~FileD();
private:
    int m_fd;
};

int set_nonblocking(int fd);
int set_tcp_nodelay(int fd);
FileD create_and_bind(std::string_view host, 
                    std::string_view port, 
                    SocketFamily family);

FileD make_listen(std::string_view host,
                std::string_view port,
                SocketFamily family,
                int backlog);

FileD accept_connection(int serversock, SocketFamily family);

ssize_t send_bytes(int fd, 
                   const std::uint8_t* buffer, 
                   std::size_t length);

ssize_t recv_bytes(int fd,
                   std::uint8_t* buffer,
                   std::size_t buffer_size);
} // namespace network

#endif
