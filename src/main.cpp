#include <stdio.h>

#include <QApplication>

#include "world.hpp"
#include "thread.hpp"
#include "main_loop.hpp"
#include "renderer/renderer.hpp"


int main( int argc, char* argv[] )
{
    QApplication hex(argc, argv);
    printf( "hex not first version\n" );
	h_MainLoop::Start();

    return hex.exec();
}
