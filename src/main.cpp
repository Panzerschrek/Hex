#include <stdio.h>

#include <QApplication>

#include "world.hpp"
#include "thread.hpp"
#include "main_loop.hpp"
#include "console.hpp"

#include "settings.hpp"

int main( int argc, char* argv[] )
{

	h_Settings ss;

	ss.SetSetting( "one", "1" );
	ss.SetSetting( "two", "two value" );

	ss.IsValue( "huj" );
	printf( "value is %s\n", ss.GetString("two" ) );


    QApplication hex(argc, argv);
	h_Console::Message( "hex not first version" );
	h_MainLoop::Start();

    return hex.exec();
}
