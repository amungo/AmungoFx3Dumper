#ifndef _tcp_common_h_include_
#define _tcp_common_h_include_

#ifdef _WIN32
#   include <winsock2.h>
#   include <ws2tcpip.h>
#else
#   include <unistd.h>
#   include <netinet/in.h>
#endif

namespace uninet {

#ifdef _WIN32

    typedef SOCKET  sock_fd_t;
    typedef int     sock_port_t;
#   define PRISOCK "zd"

#   ifdef _WIN64
        typedef __int64 ssize_t;
#   else
        typedef int ssize_t;
#   endif

#else

    typedef int     socket_fd_t;
    typedef int     port_num_t;
#   define PRISOCK "d"
#   define SOCKET_ERROR (-1)

#endif

    int     sock_close( sock_fd_t sfd );
    ssize_t sock_send( sock_fd_t sockfd, const void *buf, size_t len );
    ssize_t sock_recv( sock_fd_t sockfd, void *buf, size_t len );
    int     sock_get_last_error();
    bool    sock_is_valid( sock_fd_t sockfd );
    sock_fd_t sock_get_null_fd();

    int sock_peek( sock_fd_t sockfd ); // peeks one byte from socket stream. Returns number of read bytes

    int sock_make_non_blocking( sock_fd_t sfd );
    int sock_set_SO_REUSEADDR( sock_fd_t sfd, int flag );
    int sock_set_TCP_NODELAY( sock_fd_t sfd, int flag );
    int sock_set_SO_KEEPALIVE(sock_fd_t sfd, int flag);
    int sock_tune_KEEPALIVE( sock_fd_t sfd, int time, int interval, int probes );
}

#endif
