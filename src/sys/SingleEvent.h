#ifndef _single_event_h
#define _single_event_h

#include <mutex>
#include <condition_variable>

class SingleEvent
{
private:
    std::mutex              d_mutex;
    std::condition_variable d_condition;
    bool event = false;
public:
    inline void Wait() {
        std::unique_lock<std::mutex> lock(d_mutex);
        d_condition.wait(lock, [this]() {
            return event;
        } );
    }

    inline void Notify() {
        {
            std::unique_lock<std::mutex> lock(d_mutex);
            event = true;
        }
        d_condition.notify_all();
    }

    inline void Flush() {
        event = false;
    }
};

#endif
