#include <cstring>

#include "../console.hpp"
#include "../time.hpp"
#include "styles.hpp"
#include "ui_painter.hpp"
#include "console_menu.hpp"

ui_ConsoleMenu::ui_ConsoleMenu( ui_MenuBase* parent, unsigned int size_x, unsigned int size_y )
	: ui_MenuBase( parent, 0, 0, size_x, size_y )
	, prev_tick_time_(hGetTimeMS())
{}

ui_ConsoleMenu::~ui_ConsoleMenu(){}

void ui_ConsoleMenu::KeyPress( ui_Key key )
{
	if( key == c_activation_key )
		retracting_= !retracting_;
	else if( key == ui_Key::Escape )
		retracting_= false;
}

void ui_ConsoleMenu::KeyRelease( ui_Key key )
{
	(void)key;
}

void ui_ConsoleMenu::Tick()
{
	const uint64_t current_time= hGetTimeMS();
	const float delta= ( current_time - prev_tick_time_ ) / 100.0f * 0.5f;
	prev_tick_time_= current_time;

	if( retracting_ )
		position_= std::min( position_ + delta, 1.0f );
	else
	{
		position_= std::max( position_ - delta, 0.0f );
		if( position_ <= 0.0f )
			this->Kill();
	}
}

void ui_ConsoleMenu::Draw( ui_Painter* painter )
{
	const int x= 0, y= 0, sx= SizeX(), sy= int( position_ * float(SizeY()) * 0.5f );
	ui_Vertex triangles[6];
	triangles[0].coord[0]= x;
	triangles[0].coord[1]= y;
	triangles[1].coord[0]= x;
	triangles[1].coord[1]= y + sy;
	triangles[2].coord[0]= x + sx;
	triangles[2].coord[1]= y;

	triangles[3].coord[0]= x+sx;
	triangles[3].coord[1]= y + sy;
	triangles[4].coord[0]= x;
	triangles[4].coord[1]= y + sy;
	triangles[5].coord[0]= x + sx;
	triangles[5].coord[1]= y;

	unsigned char bg_color[4];
	std::memcpy( bg_color, c_ui_main_style.color, 3 );
	bg_color[3]= 64u + static_cast<unsigned int>(position_ * 160.0f);
	painter->DrawUITriangles( triangles, 6, bg_color );

	triangles[0].coord[1]= y + sy - 3;
	triangles[2].coord[1]= y + sy - 3;
	triangles[5].coord[1]= y + sy - 3;
	bg_color[0]/= 2u; bg_color[1]/= 2u; bg_color[2]/= 2u;
	bg_color[3]= 255u;
	painter->DrawUITriangles( triangles, 6, bg_color );

	static const unsigned char c_msg_colors[]=
	{
		0xF0, 0xF0, 0xF0, 0xFF,
		0xF0, 0xF0, 0x20, 0xFF,
		0xF0, 0x20, 0x20, 0xFF,
	};

	const auto& lines= h_Console::GetLines();

	const float line_height_px= 14.0f;
	float i= float(SizeY()) * 0.5f * position_ - line_height_px - 5.0f;
	for( auto rit= lines.rbegin();
		rit != lines.rend() && i > -1.0f;
		++rit, i-= line_height_px )
	{
		painter->DrawUITextPixelCoordsLeft(
			rit->message.data(),
			1.0f, i,
			line_height_px,
			c_msg_colors + 4 * static_cast<unsigned char>(rit->color) );
	}
}
