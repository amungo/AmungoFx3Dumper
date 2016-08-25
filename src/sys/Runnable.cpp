#include "Runnable.h"

Runnable::Runnable(std::string name) :
    NamedClass( name ),
    runnable_running( false )
{
}

Runnable::~Runnable() {
    SetIsRunningFalse();
    WaitThreadFinish();
}

void Runnable::start_run() {
    runnable_running = true;
    thread = std::thread( &Runnable::runnable_start_run_fun, this );
}

bool Runnable::IsRunning() {
    return runnable_running;
}

void Runnable::SetIsRunningFalse() {
    runnable_running = false;
}

void Runnable::WaitThreadFinish() {
    if ( thread.joinable() ) {
        thread.join();
    }
}

void Runnable::runnable_start_run_fun() {
    run();
}
