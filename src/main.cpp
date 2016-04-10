#include <clocale>

#include <QApplication>

#include "hex.hpp"
#include "main_loop.hpp"
#include "console.hpp"

int main( int argc, char* argv[] )
{
	QApplication hex(argc, argv);

	std::setlocale( LC_NUMERIC, "C" );

	h_Console::Info( "hex not first version" );
	h_MainLoop::Start();

	return hex.exec();
}
