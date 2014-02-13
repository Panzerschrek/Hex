#include <stdio.h>

#include <QApplication>

#include "world.hpp"
#include "thread.hpp"
#include "main_loop.hpp"
#include "renderer/renderer.hpp"


int main( int argc, char* argv[] )
{
    QApplication hex(argc, argv);
    printf( "hex first version\n" );

   // h_MainLoop* mainloop= new h_MainLoop( format );
	h_MainLoop::Start();

    return hex.exec();
}
