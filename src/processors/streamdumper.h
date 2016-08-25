#ifndef STREAMERDUMPER_H
#define STREAMERDUMPER_H

#ifndef WIN32
#include <stdint.h>
#else
#include <cstdint>
#endif

#include <string>
#include <mutex>
#include <cstdio>
#include <vector>

#include "sys/AsyncQueueHandler.h"
#include "hwfx3/fx3devifce.h"

enum DumperStatus_e {
    DS_PROCESS,
    DS_DONE,
    DS_ERROR   = -1
};


class StreamDumper : public DeviceDataHandlerIfce, public AsyncQueueHandler< std::vector< char > >
{
public:
    StreamDumper( const std::string& file_name, int64_t bytes_to_dump );
    ~StreamDumper();

    void HandleDeviceData(void *data_pointer, size_t size_in_bytes);

    DumperStatus_e GetStatus();
    int64_t GetBytesToGo();

protected:
    void HandleMessageAsync( std::vector< char >* message_ptr, const int current_queue_size, bool &free_ptr_flag );

private:
    int64_t bytes_to_go = 0;
    FILE* file = nullptr;

    std::mutex mtx;

    int error_count = 0;
};

#endif // STREAMERDUMPER_H
