#include <exception>
#include <iostream>

#include "socketdumper.h"

SocketDumper::SocketDumper(int port) : port(port)
{
    listen_socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    client_socket_fd = uninet::sock_get_null_fd();

    if( uninet::sock_is_valid(listen_socket_fd) < 0) {
        throw std::runtime_error("socket() failed");
    }

    if( uninet::sock_set_SO_REUSEADDR( listen_socket_fd, 1 ) < 0) {
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
    }

    struct sockaddr_in inaddr;
    inaddr.sin_family = AF_INET;
    inaddr.sin_port = htons(port);
    inaddr.sin_addr.s_addr = INADDR_ANY;

    int bindres = bind(listen_socket_fd, (struct sockaddr*) &inaddr, sizeof(inaddr));
    if( bindres < 0 ) {
        uninet::sock_close(listen_socket_fd);
        throw std::runtime_error("bind() failed");
    }

    int listenres = listen( listen_socket_fd, 1024 );

    if( listenres <  0) {
        uninet::sock_close(listen_socket_fd);
        throw std::runtime_error("listen() failed");
    }

    accept_running = true;
    accept_thread = std::thread( &AcceptRun, this );
}

SocketDumper::~SocketDumper()
{
    accept_running = false;
    if ( listen_socket_fd ) {
        uninet::sock_close( listen_socket_fd );
    }
    accept_thread.join();
    listen_socket_fd = uninet::sock_get_null_fd();
}

void SocketDumper::AcceptConnection()
{
    struct sockaddr client;
    socklen_t c = sizeof( client );
    uninet::sock_fd_t sfd = accept(listen_socket_fd, &client, (socklen_t*)&c);

    if ( uninet::sock_is_valid(sfd) ) {
        std::cerr << "SocketDumper::AcceptConnection(): Have connection" << std::endl;
        SetClientSocket(sfd);
    } else {
        std::cerr << "\nSocketDumper::AcceptConnection(): accept failed" << std::endl;
    }
}

bool SocketDumper::IsConnected()
{
    std::lock_guard<std::mutex> lck(mtx_sockets);
    return uninet::sock_is_valid(client_socket_fd);
}

void SocketDumper::SetClientSocket(uninet::sock_fd_t sfd)
{
    std::lock_guard<std::mutex> lck(mtx_sockets);
    client_socket_fd = sfd;
}

void SocketDumper::AcceptRun()
{
    try {
        while ( accept_running ) {
            AcceptConnection();
        }
    } catch ( ... ) {
        std::cerr << "SocketDumper::AcceptRun() exception (can't happen)" << std::endl;
    }
}


void SocketDumper::HandleDeviceData(void *data_pointer, size_t size_in_bytes)
{
    if ( IsConnected() ) {
        char* src = ( char* ) data_pointer;
        std::vector<char>* msg = new std::vector<char>( src, src + size_in_bytes );
        this->ReceiveMessage( msg );
    }
}

DumperIfce::DumpInfo_t SocketDumper::GetInfo()
{
    std::lock_guard<std::mutex> lck(mtx_info);
    return info;
}

void SocketDumper::HandleMessageAsync(std::vector<char> *message_ptr, const int /*current_queue_size*/, bool &free_ptr_flag)
{
    free_ptr_flag = true;
    if ( IsConnected() ) {
        ssize_t result = uninet::sock_send(client_socket_fd, message_ptr->data(), message_ptr->size());
        if ( result != (ssize_t)message_ptr->size() ) {
            std::cerr << "Socket write error: result = " << result << ", try wrote " << message_ptr->size() << " bytes" << std::endl;
        }
        {
            std::lock_guard<std::mutex> lck(mtx_info);
            info.bytes_dumped += message_ptr->size();
        }
    }
}
