#include <cstdio>
#include <fcntl.h>
#include <errno.h>
#include "TCPCommon.h"

#ifdef _WIN32
#   include <Mstcpip.h>
#else
#   include <errno.h>
#   include <netinet/tcp.h>
#endif

namespace uninet {


    #ifdef _WIN32
    class WinSockInitHelper {
    public:
        WinSockInitHelper() {
            uni_init_sockets();
        }

        //~WinSockInitHelper() { WSACleanup(); }

        static int uni_init_sockets() {
            static bool inited = false;
            if ( inited ) {
                return 0;
            }

            inited = true;

            WORD wVersionRequested;
            WSADATA wsaData;
            wVersionRequested = MAKEWORD(2,0);
            return WSAStartup(wVersionRequested, &wsaData);
        }
    };
    WinSockInitHelper wsock_init_helper;
    #endif



    int sock_close(sock_fd_t sfd) {
        int r;
    #   ifdef _WIN32
        r = closesocket( sfd );
    #   else
        r = shutdown( sfd, SHUT_RDWR );
        r = close( sfd );
    #   endif
        return r;
    }


    int sock_make_non_blocking ( sock_fd_t sfd )
    {
    #   ifdef _WIN32
        u_long mode = 1;

        int retval = ioctlsocket(sfd, FIONBIO, &mode);
        if (retval == -1) {
            int errsv = WSAGetLastError();
            if (errsv == WSANOTINITIALISED) {
                return -1;
            } else if (errsv == WSAENETDOWN) {
                return -1;
            } else if (errsv == WSAEINPROGRESS) {
                return -1;
            } else if (errsv == WSAENOTSOCK) {
                return -1;
            } else if (errsv == WSAEFAULT) {
                return -1;
            } else {
                return -1;
            }
        }
    #   else

        int flags, s;
        flags = fcntl (sfd, F_GETFL, 0);
        if (flags == -1) {
            perror ("fcntl");
            return -1;
        }
        flags |= O_NONBLOCK;
        s = fcntl (sfd, F_SETFL, flags);
        if (s == -1) {
            perror ("fcntl");
            return -1;
        }
    #   endif
        return 0;
    }


    ssize_t sock_send(sock_fd_t sockfd, const void *buf, size_t len ) {
    #   ifdef _WIN32
        return send(sockfd, (const char*)buf, len, 0);
    #   else
        return send(sockfd, buf, len, MSG_NOSIGNAL);
    #   endif
    }

    ssize_t sock_recv(sock_fd_t sockfd, void *buf, size_t len) {
    #   ifdef _WIN32
        return recv(sockfd, (char*)buf, len, 0);
    #   else
        return recv(sockfd, buf, len, MSG_NOSIGNAL);
    #   endif
    }


    int sock_peek(sock_fd_t sockfd) {
        const int size = 16;
        char buf[ size ];

        int res = 0;
    #   ifdef _WIN32
        res = recv(sockfd, buf, size, MSG_PEEK );
    #   else
        res = recv(sockfd, buf, size, MSG_PEEK | MSG_NOSIGNAL);
    #   endif
        return res;
    }

    int sock_get_last_error() {
    #   ifdef _WIN32
        int win_err = WSAGetLastError();
        switch ( win_err ) {
        case WSAEWOULDBLOCK: return EWOULDBLOCK;
        default: return win_err;
        }
    #   else
        return errno;
    #   endif
    }

    bool sock_is_valid(sock_fd_t sockfd) {
    #   ifdef _WIN32
        if ( sockfd == INVALID_SOCKET ) {
            return false;
        }
    #   else
        if ( sockfd <= 0 ) {
            return false;
        }
    #   endif
        return true;
    }

    sock_fd_t sock_get_null_fd()
    {
#   ifdef _WIN32
    return INVALID_SOCKET;
#   else
    return 0;
#   endif
    }


    int sock_set_SO_REUSEADDR(sock_fd_t sfd, int flag) {
        int yes = flag;
        return setsockopt( sfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(int) );
    }

    int sock_set_TCP_NODELAY(sock_fd_t sfd, int flag) {
        int yes = flag;
        return setsockopt( sfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&yes, sizeof(int) );
    }

    int sock_set_SO_KEEPALIVE(sock_fd_t sfd, int flag) {
        int yes = flag;
        return setsockopt( sfd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&yes, sizeof(int) );
    }

    int sock_tune_KEEPALIVE( sock_fd_t sfd, int time, int interval, int probes ) {
    #   ifdef _WIN32
        struct tcp_keepalive alive;
        DWORD dwRet, dwSize;

        alive.onoff = 1;
        alive.keepalivetime = time;
        alive.keepaliveinterval = interval;

        dwRet = WSAIoctl(sfd, SIO_KEEPALIVE_VALS, &alive, sizeof(alive), NULL, 0, reinterpret_cast<DWORD*>(&dwSize), NULL, NULL);
        if (dwRet == (DWORD)SOCKET_ERROR)
        {
            return -1;
        }
        return 0;
    #   else
        int res = 0;
        res = setsockopt( sfd, SOL_TCP, TCP_KEEPIDLE, (const char*)&time, sizeof(int) );
        if ( res ) {
            return res;
        }
        res = setsockopt( sfd, SOL_TCP, TCP_KEEPINTVL, (const char*)&interval, sizeof(int) );
        if ( res ) {
            return res;
        }
        res = setsockopt( sfd, SOL_TCP, TCP_KEEPCNT, (const char*)&probes, sizeof(int) );
        if ( res ) {
            return res;
        }
        return res;
#   endif
    }

} // namespace uninet

