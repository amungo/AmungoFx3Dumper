#ifndef SOCKETDUMPER_H
#define SOCKETDUMPER_H

#include <cstdint>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

#include "sys/AsyncQueueHandler.h"
#include "hwfx3/fx3devifce.h"
#include "processors/dumpers_ifce.h"
#include "net/TCPCommon.h"

class SocketDumper :
        public DeviceDataHandlerIfce,
        public AsyncQueueHandler< std::vector< char > >,
        public DumperIfce
{
public:
    SocketDumper( int port );
    ~SocketDumper();

    void HandleDeviceData(void *data_pointer, size_t size_in_bytes);

    virtual DumpInfo_t GetInfo();

protected:
    void HandleMessageAsync( std::vector< char >* message_ptr, const int current_queue_size, bool &free_ptr_flag );
    void AcceptConnection();
    bool IsConnected();
    void SetClientSocket( uninet::sock_fd_t sfd );
    void AcceptRun();
    void Disconnect();

private:
    std::mutex mtx_info;
    std::mutex mtx_sockets;
    int port = 0;
    DumpInfo_t info;

    uninet::sock_fd_t client_socket_fd;
    uninet::sock_fd_t listen_socket_fd;

    bool accept_running = false;
    std::thread accept_thread;
};

#endif // SOCKETDUMPER_H
