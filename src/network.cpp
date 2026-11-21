#include <cstdio>
#include <cstring>
#include <cerrno>
#include <string>
#include <utility>

#include "network.hpp"


#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

namespace Network{

FileD::~FileD(){
    if(m_fd >= 0){
        ::close(m_fd);
    }
}

// move and assignment constructors
FileD::FileD(FileD&& other) noexcept
    : m_fd(std::exchange(other.m_fd, -1)) {}

FileD& FileD::operator=(FileD&& other) noexcept {
    if(this != &other){
        if(m_fd >= 0){
            ::close(m_fd);
        }

        m_fd = std::exchange(other.m_fd, -1);
    }

    return *this;
}


FileD create_and_bind_unix(std::string_view sockpath){
    sockaddr_un addr {};

    if(sockpath.empty() || 
            sockpath.size() >= sizeof(addr.sun_path)){
        errno = ENAMETOOLONG;
        return FileD{};
    }
    
    // file class mayb
    FileD fd {socket(AF_UNIX, SOCK_STREAM, 0)};
    if(fd.get() == -1){
        std::perror("socket error");
        return FileD {}; 
    }

    addr.sun_family = AF_UNIX;
    std::memcpy(addr.sun_path, sockpath.data(), sockpath.size());
        
    std::string path {sockpath};

    // ENOENT - Error NO ENTry - no such file or directory
    if(unlink(path.c_str()) == -1 && errno != ENOENT){
        std::perror("unlink error");
        return FileD {}; // destructor activates
    }

    if(bind(fd.get(),
             reinterpret_cast<sockaddr*>(&addr),
             sizeof(addr)) == -1){
        std::perror("bind error");
        return FileD {}; // same here
    }
    
    return fd; // transfers ownship to caller
        
    }

FileD create_and_bind_tcp(std::string_view host, std::string_view port){
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
        std::fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return FileD {};
    }


    int last_error = EADDRNOTAVAIL;
    FileD fd {};

    for(addrinfo* rp = result; rp != nullptr; rp = rp->ai_next){
        fd = FileD{socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)};

        if(fd.get() == -1){
            continue;
        }

        int enable = 1;

        int sock_opt = setsockopt(fd.get(),
                                  SOL_SOCKET,
                                  SO_REUSEADDR,
                                  &enable,
                                  sizeof(enable));
        if(sock_opt == -1){
            std::perror("SO_REUSEADDR");
            fd = FileD {};
            continue;
        }

        if(bind(fd.get(), rp->ai_addr, rp->ai_addrlen) == 0){
            break;
        }

   last_error = errno;
        fd = FileD {};
    }

    freeaddrinfo(result);

    if(fd.get() == -1){
        errno = last_error;
        std::perror("could not bind");
    }
    
    return fd;
}

int set_nonblocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);

    if(flags == -1){
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

FileD create_and_bind(std::string_view host, 
                      std::string_view port, 
                      SocketFamily family)
{
    if(family == SocketFamily::Unix){
        return create_and_bind_unix(host);
    }
    else{
        return create_and_bind_tcp(host, port);
    }
}

FileD make_listen(std::string_view host,
                  std::string_view port,
                  SocketFamily family,
                  int backlog)
{
    FileD fd {create_and_bind(host, port, family)};

    if(fd.get() == -1){
        return FileD {};
    }

    if(set_nonblocking(fd.get())  == -1){
        return FileD {};
    }

    if(listen(fd.get(), backlog)){
        std::perror("listen");
        return FileD {};
    }

    return fd;

}

FileD accept_connection(int serversock, SocketFamily family){
    sockaddr_storage their_addr {};
    socklen_t addr_size = sizeof(their_addr);

    FileD client {
        accept(serversock,
               reinterpret_cast<sockaddr*>(&their_addr),
               &addr_size
        )
    };

    if(client.get() == -1){
        if(errno == EAGAIN || errno == EWOULDBLOCK){
            return FileD {};
        }

        std::perror("accept failure");
        return FileD {};
    }

   if(set_nonblocking(client.get()) == -1){
       return FileD {};
   }

   if(family == SocketFamily::Inet){
       if(set_tcp_nodelay(client.get()) == -1){
           std::perror("TCP_NODELAY ERROR");
           return FileD {};
        }
    }


   return client;
    
}


}

