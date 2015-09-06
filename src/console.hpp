#pragma once
#define H_CONSOLE_BUFFER_LEN 4096
#define H_CONSOLE_BUFFER_MAX_LINES (H_CONSOLE_BUFFER_LEN/2)
#define H_CONSOLE_MOVING_SPEED 1.0f

class r_Text;

class h_Console
{
public:

	inline static float GetPosition();

	inline static void Toggle();//open or close console
	inline static void Move( float dt );//roll up or roll down colsole

	static void Message( const char* str, ... );
	static void Warning( const char* str, ...  );
	static void Error( const char* str, ... );

	static void Draw( r_Text* text );

private:

enum ConsoleColor:
	unsigned char
	{
		WHITE,
		YELLOW,
		RED
	};

	static void WriteText( const char* str, ConsoleColor color );

	static char buffer[ H_CONSOLE_BUFFER_LEN ];
	static char* lines_beginning[ H_CONSOLE_BUFFER_MAX_LINES ];//every line contains 1 or more symbols
	static ConsoleColor lines_color_id[ H_CONSOLE_BUFFER_MAX_LINES ];

	static unsigned int buffer_pos, lines_buffer_pos;

	static bool initialized;
	static bool is_ingame_console;

	static float moving_direction;
	static float position;// 0 - closed, 1 - opened, (0;1) - rolling-up \ rolling-down

};

inline void h_Console::Toggle()
{
	if( moving_direction != 1.0f )
		moving_direction= 1.0f;
	else
		moving_direction= -1.0f;
}
inline void h_Console::Move( float dt )
{
	position+= moving_direction * dt;
	if( position > 1.0f ) position= 1.0f;
	else if( position < 0.0f ) position= 0.0f;
}

inline float h_Console::GetPosition()
{
	return position;
}
