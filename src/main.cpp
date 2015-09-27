#include <QApplication>

#include "hex.hpp"
#include "main_loop.hpp"
#include "console.hpp"

#include "world_generator/world_generator.hpp"

int main( int argc, char* argv[] )
{
	g_WorldGenerationParameters parameters;
	g_WorldGenerator generator(parameters);
	generator.Generate();

	QApplication hex(argc, argv);
	h_Console::Message( "hex not first version" );
	h_MainLoop::Start();

	return hex.exec();
}
