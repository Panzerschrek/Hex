#include "settings.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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

h_Settings::h_Settings( const char* file_name )
	: file_name_(file_name)
{
	QFile f( QString::fromStdString(file_name_) );
	if( !f.open( QIODevice::ReadOnly ) )
	{
		h_Console::Warning( "can not read file \"", file_name_, "\"" );
		return;
	}

	QByteArray ba= f.readAll();
	f.close();

	QJsonDocument doc= QJsonDocument::fromJson( ba );

	if (doc.isObject())
	{
		QJsonObject obj= doc.object();

		for( auto cit= obj.constBegin(); cit != obj.constEnd(); cit++ )
			map_[ h_SettingsStringContainer(cit.key().toStdString().data()) ] = cit.value().toString().toStdString();
	}
}

h_Settings::~h_Settings()
{
	QJsonObject obj;
	for( const auto& key_value_pair : map_ )
		obj[ QString::fromStdString(std::string(key_value_pair.first)) ]=
			QString::fromStdString(key_value_pair.second);

	QJsonDocument doc(obj);
	QByteArray ba= doc.toJson();

	QFile f( QString::fromStdString(file_name_) );
	if( !f.open( QIODevice::WriteOnly ) )
	{
		h_Console::Warning( "can not write file \"", file_name_, "\"" );
		return;
	}

	f.write( ba );
	f.close();
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

h_Settings::h_SettingsStringContainer::operator std::string() const
{
	return c_str_ ? std::string(c_str_) : str_;
}

h_Settings::h_SettingsStringContainer::~h_SettingsStringContainer()
{
}

bool h_Settings::h_SettingsStringContainer::operator < ( const h_SettingsStringContainer& other ) const
{
	const char* this_c_str= c_str_ ? c_str_ : str_.c_str();
	const char* other_c_str= other.c_str_ ? other.c_str_ : other.str_.c_str();
	return strcmp( this_c_str, other_c_str ) < 0;
}
