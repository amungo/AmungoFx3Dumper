#ifndef _async_queue_handler_
#define _async_queue_handler_

#include "sys/Runnable.h"
#include "sys/MessageReceiverIfce.h"
#include "sys/BlockQueue.h"

template< class T >
class AsyncQueueHandler:
        public Runnable,
        public MessageReceiverIfce< T >
{
public:
    AsyncQueueHandler( std::string name = "" ) : Runnable( name ) {
        this->start_run();
    }

    ~AsyncQueueHandler() {
        SetIsRunningFalse();
        T* stopMsg = new T();
        this->ReceiveMessage( stopMsg );
        this->WaitThreadFinish();
        ClearQueue();
    }
protected:
    virtual void HandleMessageAsync( T* message_ptr, const int current_queue_size, bool &free_ptr_flag ) = 0;

public:
    // MessageReceiverIfce interface
    void ReceiveMessage( T* message_ptr ) final {
        queue.push( message_ptr );
    }
protected:
    void SetOverflow() {
        isOverflow = true;
    }

private:
    // Runnable interface
    void run() final {
        while ( IsRunning() ) {
            T* message_ptr = queue.getFrontBlocked();
            if (!IsRunning()) {
                return;
            } else {
                bool free_ptr_flag = true;
                HandleMessageAsync( message_ptr, queue.getSize(), free_ptr_flag );
                if ( free_ptr_flag ) {
                    delete message_ptr;
                }
                queue.pop();
                if ( isOverflow ) {
                    isOverflow = false;
                    ClearQueue();
                }
            }
        }
    }

    void ClearQueue() {
        while ( !queue.empty() ) {
            delete queue.getFrontUnblocked();
            queue.pop();
        }
    }    

private:
    BlockQueue< T* > queue;
    bool isOverflow = false;
};


#endif


