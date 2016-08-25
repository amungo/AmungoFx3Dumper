#ifndef _message_receiver_ifce_h_
#define _message_receiver_ifce_h_

template <class T>
class MessageReceiverIfce {
public:
    virtual void ReceiveMessage( T* message_ptr ) = 0;
};


#endif
