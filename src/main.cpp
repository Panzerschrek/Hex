#include <clocale>

#include <SDL.h>

#include "hex.hpp"
#include "main_loop.hpp"
#include "console.hpp"

extern "C" int main( int argc, char *argv[] )
{
	(void) argc;
	(void) argv;

	h_Console::Info( "hex not first version" );

	std::setlocale( LC_NUMERIC, "C" );

	h_MainLoop main_loop;
	while(main_loop.Loop())
	{}
}
