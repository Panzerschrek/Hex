#include <clocale>

#include <QApplication>
#include <QDir>

#include "hex.hpp"
#include "main_loop.hpp"
#include "console.hpp"

int main( int argc, char* argv[] )
{
	QApplication app( argc, argv );

	// We can not work outside executable directory.
	//QDir::setCurrent( app.applicationDirPath() );

	std::setlocale( LC_NUMERIC, "C" );

	h_Console::Info( "hex not first version" );
	h_MainLoop::Start();

	return app.exec();
}
