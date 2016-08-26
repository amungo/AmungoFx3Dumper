#include "streamdumper.h"

#include <cstdio>
#include <iostream>

using namespace std;

StreamDumper::StreamDumper(const std::string &file_name, int64_t bytes_to_dump ) :
    bytes_to_go( bytes_to_dump )
{
    file = fopen( file_name.c_str(), "wb" );
    if ( !file ) {
        throw std::runtime_error( "StreamDumper::StreamDumper() error opening file '" + file_name + "'" );
    }
}

StreamDumper::~StreamDumper() {
    if ( file ) {
        fclose( file );
        cout << "File was closed" << endl;
    }
}

void StreamDumper::HandleDeviceData(void *data_pointer, size_t size_in_bytes)
{
    char* src = ( char* ) data_pointer;
    std::vector<char>* msg = new std::vector<char>( src, src + size_in_bytes );
    this->ReceiveMessage( msg );
}

DumperStatus_e StreamDumper::GetStatus() {
    std::lock_guard<std::mutex> guard( mtx );
    if ( error_count ) {
        return DS_ERROR;
    }
    if ( bytes_to_go <= 0 ) {
        return DS_DONE;
    }
    return DS_PROCESS;
}

int64_t StreamDumper::GetBytesToGo() {
    std::lock_guard<std::mutex> guard( mtx );
    return bytes_to_go;
}

void StreamDumper::HandleMessageAsync(std::vector<char> *message_ptr, const int current_queue_size, bool &free_ptr_flag) {
    free_ptr_flag = true;
    {
        std::lock_guard<std::mutex> guard( mtx );
        if ( bytes_to_go < 0 ) {
            return;
        }
        bytes_to_go -= message_ptr->size();
    }
    if ( file ) {
        size_t wrote = fwrite( message_ptr->data(), 1, message_ptr->size(), file );
        if ( wrote != message_ptr->size() ) {
            cerr << "File write error: wrote " << wrote << " of " << message_ptr->size() << " bytes" << endl;
            error_count++;
        }
    }
}



