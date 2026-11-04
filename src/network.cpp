#include <cstdio>
#include <cstring>
#include <cerrno>
#include <string>
#include <utility>

#include "network.hpp"


#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>

namespace Network{

file_d::~file_d(){
    if(m_fd >= 0){
        ::close(m_fd);
    }
}

// move and assignment constructors
file_d::file_d(file_d&& other) noexcept
    : m_fd(std::exchange(other.m_fd, -1)) {}

file_d& file_d::operator=(file_d&& other) noexcept {
    if(this != &other){
        if(m_fd >= 0){
            ::close(m_fd);
        }

        m_fd = std::exchange(other.m_fd, -1);
    }

    return *this;
}


int create_and_bind_unix(std::string_view sockpath){
    sockaddr_un addr {};

    if(sockpath.empty() || 
            sockpath.size() >= sizeof(addr.sun_path)){
        errno = ENAMETOOLONG;
        return -1;
    }
    
    // file class mayb
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd == -1){
        std::perror("socket error");
        return -1;
    }

    addr.sun_family = AF_UNIX;
    std::memcpy(addr.sun_path, sockpath.data(), sockpath.size());
        
    std::string path {sockpath};

    if(unlink(path.c_str()) == -1 && errno != ENOENT){
        std::perror("unlink error");
        close(fd);
        return -1;
    }

    if( bind(fd,
             reinterpret_cast<sockaddr*>(&addr),
             sizeof(addr)) == -1){
        std::perror("bind error");
        close(fd);
        return -1;
    }
    
    return fd;
        
    }
set_nonblocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);

    if(flahs == -1){
        std::perror("fcntl F_GETFL");
        return -1;
    }

    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1){
        std::perror("fcntl F_SETFL");
        return -1;
    }

    return 0;
}
int set_tcp_nodelay(int fd){
    int enable = 1;

    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
}


}

