#ifndef POINTDRAWER_H
#define POINTDRAWER_H

#include <thread>

class PointDrawer
{
public:
    PointDrawer( int milliseconds = 750 );
    ~PointDrawer();
protected:
    std::thread drawer;
    bool drawing;
};

#endif // POINTDRAWER_H
