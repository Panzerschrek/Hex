#include <cmath>

#include "hex.hpp"
#include "console.hpp"
#include "renderer/text.hpp"

#include "framebuffer.hpp"

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

void h_Console::Toggle()
{
	std::lock_guard<std::mutex> lock(mutex_);

	if( moving_direction_ < 1.0f )
		moving_direction_= 1.0f;
	else
		moving_direction_= -1.0f;
}

void h_Console::Move( float dt )
{
	std::lock_guard<std::mutex> lock(mutex_);

	position_+= moving_direction_ * dt;
	if( position_ > 1.0f ) position_= 1.0f;
	else if( position_ < 0.0f ) position_= 0.0f;
}

float h_Console::GetPosition()
{
	std::lock_guard<std::mutex> lock(mutex_);

	return position_;
}

void h_Console::Draw( r_Text* text )
{
	std::lock_guard<std::mutex> lock(mutex_);

	static const unsigned char c_msg_colors[]=
	{
		0xF0, 0xF0, 0xF0, 0xFF,
		0xF0, 0xF0, 0x20, 0xFF,
		0xF0, 0x20, 0x20, 0xFF
	};

	const float c_font_scale= 0.25f;

	float i=
		0.5f * position_ * float(r_Framebuffer::CurrentFramebufferHeight()) / (float(text->LetterHeight()) * c_font_scale )
		- 1.5f;
	for( auto rit= lines_.rbegin();
		rit != lines_.rend() && i > -1.0f;
		++rit, i-= 1.0f )
	{
		text->AddText(
			0.5f, i,
			c_font_scale,
			c_msg_colors + 4 * static_cast<unsigned char>(rit->color),
			rit->message.data() );
	}
}
