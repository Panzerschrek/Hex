#include <clocale>


#include <SDL.h>
//#include <QApplication>
//#include <QDir>

#include "hex.hpp"
#include "main_loop.hpp"
#include "console.hpp"

extern "C" int main( int argc, char *argv[] )
{
	//QApplication app( argc, argv );

	// We can not work outside executable directory.
	//QDir::setCurrent( app.applicationDirPath() );

	std::setlocale( LC_NUMERIC, "C" );

	h_Console::Info( "hex not first version" );

	h_MainLoop main_loop;
	while(main_loop.Loop())
	{}

	//return app.exec();
}
