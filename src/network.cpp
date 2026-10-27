#include <cstdio>
#include <cstring>
#include <cerrno>


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

file_d& file_d::file_d::operator=(file_d&& other) noexcept {
    if(this != other){
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
        std::perror("socket");
        return -1;
    }

    addr.sun_family = AF_UNIX;
    std::memcpy(addr.sun_path, sockpath.data(), sockpath.size());
        
    std::string path {sockpath};

    if(unlink(path.c_str()) == -1 && errno != ENOENT){
        std::perror("unlink");
        close(fd);
        return -1;
    }

    if( bind(fd,
             reinterpret_cast<sockaddr*>(&addr),
             sizeof(addr)) == -1){
        std::perror("bind");
        close(fd);
        return -1;
    }
    
    return fd;
        
    }
}

