#ifndef BORA_BLOCKQUEUE_H
#define BORA_BLOCKQUEUE_H

#include <mutex>
#include <condition_variable>
#include <deque>

template <typename T>
class BlockQueue
{
private:
    std::mutex              d_mutex;
    std::condition_variable d_condition;
    std::deque<T>           d_queue;
public:
    void push(T const& value) {
        {
            std::unique_lock<std::mutex> lock(d_mutex);
            d_queue.push_back(value);
        }
        d_condition.notify_one();
    }

    void pop() {
        std::unique_lock<std::mutex> lock(d_mutex);
        d_queue.pop_front();
    }

    T& getFrontBlocked() {
        std::unique_lock<std::mutex> lock(d_mutex);
        d_condition.wait(lock, [this](){ return !d_queue.empty(); } );
        return d_queue.front();
    }

    T& getFrontUnblocked() {
        return d_queue.front();
    }

    inline bool empty() {
        return d_queue.empty();
    }

    int getSize() {
        return d_queue.size();
    }

};

#endif //BORA_BLOCKQUEUE_H
