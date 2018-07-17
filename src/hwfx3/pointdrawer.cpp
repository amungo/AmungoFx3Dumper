#include <chrono>
#include <iostream>

#include "pointdrawer.h"

PointDrawer::PointDrawer( int milliseconds ) {

    drawing = true;
    drawer = std::thread([=](){
        while (drawing) {
            std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
            std::cerr << ".";
        }
    } );

}

PointDrawer::~PointDrawer() {
    drawing = false;
    if ( drawer.joinable() ) {
        drawer.join();
    }
}

