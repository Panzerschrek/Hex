#include <fstream>

#include <PanzerJson/parser.hpp>
#include <PanzerJson/streamed_serializer.hpp>

#include "settings.hpp"

#include "console.hpp"

static bool hStrToInt( const char* str, int* i )
{
	int sign = 1;
	if( str[0] == '-' )
	{
		sign = -1;
		str++;
	}

	int v = 0;
	while( *str != 0 )
	{
		if( str[0] < '0' || str[0] > '9' )
			return false;
		v*= 10;
		v+= str[0] - '0';
		str++;
	}
	*i= v * sign;
	return true;
}

static bool hStrToFloat( const char* str, float* f )
{
	float sign = 1.0f;
	if( str[0] == '-' )
	{
		sign = -1.0f;
		str++;
	}

	float v = 0;
	while( *str != 0 )
	{
		if( str[0] == ',' || str[0] == '.' )
		{
			str++;
			break;
		}
		if( str[0] < '0' || str[0] > '9' )
			return false;
		v*= 10.0f;
		v+= float(str[0] - '0');
		str++;
	}
	float m = 0.1f;
	while( *str != 0 )
	{
		if( str[0] < '0' || str[0] > '9' )
			return false;

		v+= float(str[0] - '0') * m;
		m*= 0.1f;
		str++;
	}

	*f= v * sign;
	return true;
}

h_Settings::h_Settings( const char* const file_name )
	: file_name_(file_name)
{
	std::FILE* const f= std::fopen( file_name , "rb" );
	if( f == nullptr )
		return;

	std::fseek( f, 0, SEEK_END );
	const size_t file_size= std::ftell( f );
	std::fseek( f, 0, SEEK_SET );

	std::vector<char> file_content( file_size );
	std::fread( file_content.data(), 1, file_size, f ); // TODO - check file errors
	std::fclose(f);

	const PanzerJson::Parser::ResultPtr parse_result=
		PanzerJson::Parser().Parse( file_content.data(), file_content.size() );
	if( parse_result->error != PanzerJson::Parser::Result::Error::NoError )
	{
		h_Console::Error( "Error, parsing json" );
		return;
	}

	for( const auto key_value_pair : parse_result->root.object_elements() )
		map_[ h_SettingsStringContainer(key_value_pair.first) ] = key_value_pair.second.AsString();
}

h_Settings::~h_Settings()
{
	std::ofstream stream( file_name_ );

	PanzerJson::StreamedSerializer<std::ofstream> serializer( stream );
	auto obj= serializer.AddObject();

	for( const auto& key_value_pair : map_ )
	{
		obj.AddString( key_value_pair.first.c_str(), key_value_pair.second.c_str() );
	}
}

void h_Settings::SetSetting( const char* name, const char* value )
{
	map_[ h_SettingsStringContainer(name) ]= std::string(value);
}

void h_Settings::SetSetting( const char* name, int value )
{
	map_[ h_SettingsStringContainer(name) ]= std::to_string(value);
}

void h_Settings::SetSetting( const char* name, bool value )
{
	map_[ h_SettingsStringContainer(name) ]= std::to_string( int(value) );
}

void h_Settings::SetSetting( const char* name, float value )
{
	// HACK - replace ',' to '.' for bad locale
	std::string str = std::to_string( value );
	size_t pos = str.find(",");
	if ( pos != std::string::npos ) str[pos] = '.';

	map_[ h_SettingsStringContainer(name) ]= str;
}

bool h_Settings::IsValue( const char* name ) const
{
	auto it = map_.find( h_SettingsStringContainer(name) );
	return it != map_.cend();
}

bool h_Settings::IsNumber( const char* name ) const
{
	auto it = map_.find( h_SettingsStringContainer(name) );
	if ( it == map_.cend() )
		return false;

	float f;
	return hStrToFloat( (*it).second.data(), &f );
}

const char* h_Settings::GetString( const char* name, const char* default_value ) const
{
	auto it = map_.find( h_SettingsStringContainer(name) );
	if ( it == map_.cend() )
		return default_value;

	return (*it).second.data();
}

int h_Settings::GetInt( const char* name, int default_value ) const
{
	auto it = map_.find( h_SettingsStringContainer(name) );
	if ( it == map_.cend() )
		return default_value;

	int val;
	if( hStrToInt( (*it).second.data(), &val ) )
		return val;
	return default_value;
}

float h_Settings::GetFloat( const char* name, float default_value ) const
{
	auto it = map_.find( h_SettingsStringContainer(name) );
	if ( it == map_.cend() )
		return default_value;

	float val;
	if( hStrToFloat( (*it).second.data(), &val ) )
		return val;
	return default_value;
}

bool h_Settings::GetBool( const char* name, bool default_value ) const
{
	return GetInt( name, int(default_value) );
}

/*
------------------h_Settings::h_SettingsStringContainer-----------------------
*/

h_Settings::h_SettingsStringContainer::h_SettingsStringContainer( const char* str )
	: c_str_(str), str_()
{
}

h_Settings::h_SettingsStringContainer::h_SettingsStringContainer( const h_SettingsStringContainer& other )
	: c_str_(nullptr)
	, str_( other.c_str_ ? other.c_str_ : other.str_ )
{
}

h_Settings::h_SettingsStringContainer::~h_SettingsStringContainer()
{
}

const char* h_Settings::h_SettingsStringContainer::c_str() const
{
	return c_str_ == nullptr ? str_.c_str() : c_str_;
}

bool h_Settings::h_SettingsStringContainer::operator < ( const h_SettingsStringContainer& other ) const
{
	const char* this_c_str= c_str_ ? c_str_ : str_.c_str();
	const char* other_c_str= other.c_str_ ? other.c_str_ : other.str_.c_str();
	return strcmp( this_c_str, other_c_str ) < 0;
}
