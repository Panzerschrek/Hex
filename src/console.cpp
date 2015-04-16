#ifndef CONSOLE_CPP
#define CONSOLE_CPP

#include "console.hpp"
#include "hex.hpp"
#include "renderer/text.hpp"

char h_Console:: buffer[ H_CONSOLE_BUFFER_LEN ];
char* h_Console:: lines_beginning[ H_CONSOLE_BUFFER_MAX_LINES ];
h_Console::ConsoleColor h_Console:: lines_color_id[ H_CONSOLE_BUFFER_MAX_LINES ];

unsigned int h_Console:: buffer_pos=0;
unsigned int h_Console:: lines_buffer_pos=0;
bool h_Console:: initialized=false;
bool h_Console:: is_ingame_console= false;

float h_Console::moving_direction= 0.0f;
float h_Console::position= 0.0f;

static char tmp_buffer[ H_CONSOLE_BUFFER_LEN ];
void h_Console::Message( const char* str, ... )
{
    va_list ap;
    va_start( ap, str );
    vsprintf( tmp_buffer, str, ap );
    va_end( ap );

	WriteText( tmp_buffer, WHITE );
}
void h_Console::Warning( const char* str, ...  )
{
    va_list ap;
    va_start( ap, str );
    vsprintf( tmp_buffer, str, ap );
    va_end( ap );

	WriteText( tmp_buffer, YELLOW );
}
void h_Console::Error( const char* str, ...  )
{
    va_list ap;
    va_start( ap, str );
    vsprintf( tmp_buffer, str, ap );
    va_end( ap );

	WriteText( tmp_buffer, RED );
}


void h_Console::WriteText( const char* str, ConsoleColor color )
{
	lines_beginning[ lines_buffer_pos % H_CONSOLE_BUFFER_MAX_LINES ]= &buffer[ buffer_pos % H_CONSOLE_BUFFER_LEN ];
	lines_color_id[ lines_buffer_pos % H_CONSOLE_BUFFER_MAX_LINES ]= color;
	lines_buffer_pos++;

	const char* s= str;
	while( s[0] != 0 )
	{
		if( s[0] == '\n' )
		{
			buffer[ buffer_pos % H_CONSOLE_BUFFER_LEN ]= 0x00;
			buffer_pos++;

			lines_beginning[ lines_buffer_pos % H_CONSOLE_BUFFER_MAX_LINES ]= &buffer[ buffer_pos % H_CONSOLE_BUFFER_LEN ];
			lines_color_id[ lines_buffer_pos % H_CONSOLE_BUFFER_MAX_LINES ]= color;
			lines_buffer_pos++;
		}
		else
		{
			buffer[ buffer_pos % H_CONSOLE_BUFFER_LEN ]= s[0];
			buffer_pos++;
		}
		s++;
	}
	//write end of line
	buffer[ buffer_pos % H_CONSOLE_BUFFER_LEN ]= 0x00;
	buffer_pos++;

	if( ! is_ingame_console )
		printf( "%s\n", str );
}


void h_Console::Draw( r_Text* text )
{
	is_ingame_console= true;

	if( position == 0.0f )
		return;

	float init_row= 4.0f * ( 0.5f * position * float( text->RowsInScreen() ) );
	float i; int j;
	for( i= init_row, j= lines_buffer_pos - 1; i>= 0.0f && j>=0 ; i-=1.0f, j-- )
	{
		const unsigned char* c;
		static const unsigned char msg_colors[]= {
			 255, 255, 255, 0,
			 255, 32, 32, 0,
			 255, 255, 32, 0 };
		int l= j % H_CONSOLE_BUFFER_MAX_LINES;
		if( lines_color_id[l] == RED )
			c= msg_colors + 4;
		else if( lines_color_id[l] == YELLOW )
			c= msg_colors + 8;
		else
			c= msg_colors;
		text->AddText( 0.5f, i, 0.25f, c, lines_beginning[ l ] );
	}
}

#endif//CONSOLE_CPP
