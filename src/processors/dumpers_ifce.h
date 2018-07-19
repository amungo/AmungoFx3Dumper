#ifndef DUMPERIFCE_H
#define DUMPERIFCE_H

#include <cstdint>

class DumperIfce {
public:
    virtual ~DumperIfce() = default;
    struct DumpInfo_t {
        int error_count = 0;
        int64_t bytes_dumped = 0;
        int64_t bytes_rcvd   = 0;
    };

    virtual DumpInfo_t GetInfo() = 0;
};


#endif
