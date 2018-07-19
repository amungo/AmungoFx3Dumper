#include <cstdio>
#include <iostream>

#include "streamdumper.h"

using namespace std;

StreamDumper::StreamDumper(const std::string &file_name )
{
    if ( file_name == std::string( "stdout" ) ) {
        use_stdout = true;
        file = stdout;
    } else {
        use_stdout = false;
        file = fopen( file_name.c_str(), "wb" );
    }
    if ( !file ) {
        throw std::runtime_error( "StreamDumper::StreamDumper() error opening file '" + file_name + "'" );
    }
}

StreamDumper::~StreamDumper() {
    if ( file ) {
        if ( use_stdout ) {
            fflush( stdout );
            cerr << "Stream was flushed" << endl;
        } else {
            fclose( file );
            cerr << "File was closed" << endl;
        }
    }
}

void StreamDumper::HandleDeviceData(void *data_pointer, size_t size_in_bytes)
{
    info.bytes_rcvd += size_in_bytes;

    char* src = ( char* ) data_pointer;
    std::vector<char>* msg = new std::vector<char>( src, src + size_in_bytes );
    this->ReceiveMessage( msg );
}

StreamDumper::DumpInfo_t StreamDumper::GetInfo() {
    std::lock_guard<std::mutex> lck(mtx_info);
    return info;
}

void StreamDumper::HandleMessageAsync(std::vector<char> *message_ptr, const int /*current_queue_size*/, bool &free_ptr_flag) {
    free_ptr_flag = true;
    if ( file ) {
        size_t wrote = fwrite( message_ptr->data(), 1, message_ptr->size(), file );
        {
            std::lock_guard<std::mutex> lck(mtx_info);
            if ( wrote != message_ptr->size() ) {
                cerr << "File write error: wrote " << wrote << " of " << message_ptr->size() << " bytes" << endl;
                info.error_count++;
            }
            info.bytes_dumped += message_ptr->size();
        }
    }
}



