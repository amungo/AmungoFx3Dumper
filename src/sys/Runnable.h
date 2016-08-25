#ifndef _core_runnable_h_
#define _core_runnable_h_

#include <thread>
#include <string>

#include "sys/NamedClass.h"

class Runnable : public NamedClass {
public:
    Runnable( std::string name = "noname_runnable" );

    virtual ~Runnable();

protected:
    virtual void start_run() final;
    virtual bool IsRunning() final;
    virtual void SetIsRunningFalse() final;
    virtual void WaitThreadFinish() final;
    virtual void run() = 0;
private:
    void runnable_start_run_fun();

    bool runnable_running;
    std::thread thread;
};

#endif
