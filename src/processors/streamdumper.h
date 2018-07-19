#ifndef STREAMERDUMPER_H
#define STREAMERDUMPER_H

#include <cstdint>
#include <string>
#include <mutex>
#include <cstdio>
#include <vector>

#include "sys/AsyncQueueHandler.h"
#include "hwfx3/fx3devifce.h"
#include "processors/dumpers_ifce.h"

class StreamDumper :
        public DeviceDataHandlerIfce,
        public AsyncQueueHandler< std::vector< char > >,
        public DumperIfce
{
public:
    StreamDumper( const std::string& file_name );
    ~StreamDumper();

    void HandleDeviceData(void *data_pointer, size_t size_in_bytes);

    virtual DumpInfo_t GetInfo();

protected:
    void HandleMessageAsync( std::vector< char >* message_ptr, const int current_queue_size, bool &free_ptr_flag );

private:
    std::mutex mtx_info;
    FILE* file = nullptr;
    DumpInfo_t info;
    bool use_stdout = false;
};

#endif // STREAMERDUMPER_H
