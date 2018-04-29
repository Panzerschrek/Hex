#include <cmath>

#include "hex.hpp"
#include "console.hpp"

std::mutex h_Console::mutex_;
std::stringstream h_Console::stream_;
std::list<h_Console::MessageLine> h_Console::lines_;

float h_Console::moving_direction_= 0.0f;
float h_Console::position_= 0.0f;

h_Console::MessageLine::MessageLine( h_Console::Color in_color, std::string in_message )
	: color(in_color)
	, message(std::move(in_message))
{}

h_Console::MessageLine::MessageLine( h_Console::MessageLine&& other )
{
	*this= std::move(other);
}

h_Console::MessageLine& h_Console::MessageLine::operator=( h_Console::MessageLine&& other )
{
	color= other.color;
	message= std::move(other.message);
	return *this;
}
