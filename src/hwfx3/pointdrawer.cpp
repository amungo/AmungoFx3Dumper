#include "pointdrawer.h"

#include <chrono>
#include <iostream>

PointDrawer::PointDrawer( int milliseconds ) {

    drawing = true;
    drawer = std::thread([&](){
        while (drawing) {
            std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
            std::cout << ".";
        }
    } );

}

PointDrawer::~PointDrawer() {
    drawing = false;
    if ( drawer.joinable() ) {
        drawer.join();
    }
}

