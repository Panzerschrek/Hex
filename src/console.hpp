#pragma once
#include <iostream>
#include <list>
#include <mutex>
#include <sstream>

#include "fwd.hpp"

#define H_CONSOLE_MAX_LINES 512

// Static class for console output.
// Thread safe.
class h_Console
{
public:
	static float GetPosition();
	static void Toggle();//open or close console
	static void Move( float dt );//roll up or roll down colsole

public:
	template<class ... Args>
	static void Info( const Args &... args )
	{
		MessageHandler( Color::White, args... );
	}

	template<class ... Args>
	static void Warning( const Args &... args )
	{
		MessageHandler( Color::Yellow, args... );
	}

	template<class ... Args>
	static void Error( const Args &... args )
	{
		MessageHandler( Color::Red, args... );
	}

	static void Draw( r_Text* text );

private:

	enum class Color : unsigned char
	{
		White,
		Yellow,
		Red,
	};

	struct MessageLine
	{
		Color color;
		std::string message;

		MessageLine( Color in_color, std::string in_message );
		MessageLine( MessageLine&& other );

		MessageLine& operator=( const MessageLine& )= delete;
		MessageLine& operator=( MessageLine&& other );
	};

private:
	static void MessageExpand(){}

	template<class Arg0, class ... Args>
	static void MessageExpand( const Arg0& arg0, const Args &... args )
	{
		stream_ << arg0;
		MessageExpand(args...);
	}

	template<class ... Args>
	static void MessageHandler( Color color, const Args &... args )
	{
		std::lock_guard<std::mutex> lock(mutex_);

		if( lines_.size() > H_CONSOLE_MAX_LINES ) lines_.pop_front();

		MessageExpand(args...);

		std::string str= stream_.str();
		std::cout << str << std::endl;

		lines_.emplace_back( color, std::move(str) );

		stream_.str( "" );
	}

private:
	static std::mutex mutex_;
	static std::stringstream stream_;
	static std::list<MessageLine> lines_;

	static float moving_direction_;
	static float position_;// 0 - closed, 1 - opened, (0;1) - rolling-up \ rolling-down
};
