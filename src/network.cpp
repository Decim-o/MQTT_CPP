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

int create_and_bind_tcp(std::string_view host, std::string_view port){
    addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* result = nullptr;

    std::string host_str {host};
    std::string port_str {port};

    const char* hostname = host.empty() ? nullptr : host_str.c_str();

    int status = getaddrinfo(hostname, port_str.c_str(), &hints, &result);

    if(status != 0){
        std::fprintf(stderr, "getaddrinfo: %s\n", gai_streroor(status));
        return -1;
    }

    int fd = -1;

    for(addrinfo* rp = result; rp != nullptr; rp->ai_next){
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if(fd == -1){
            continue;
        }

        int enable = 1;

        int sock_opt = setsockopt(fd,
                                  SOL_SOCKET,
                                  SO_REUSEADDR,
                                  &enable,
                                  sizeof(enable));
        if(sock_opt == -1){
            std::perror("SO_REUSEADDR");
            close(fd);
            fd = -1;
            continue;
        }

        if(bind(fd, rp->ai_addr, rp->ai_addrlen) == 0){
            break;
        }

        close(fd);
        fd = -1;
    }

    freeaddrinfo(result);

    if(fd == -1){
        std::perror("could not bind");
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

