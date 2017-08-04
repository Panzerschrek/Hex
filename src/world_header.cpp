#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "console.hpp"
#include "world_header.hpp"

static const char g_world_header_filename[]= "world.json";

static std::string GetFullFileName( const char* const world_dir )
{
	return std::string(world_dir) + "/" + g_world_header_filename;
}

void h_WorldHeader::Load( const char* world_dir )
{
	QFile file( QString::fromStdString(GetFullFileName(world_dir)) );
	if( !file.open( QIODevice::ReadOnly ) )
	{
		h_Console::Error( "Can not open world header file ", g_world_header_filename );
		return;
	}

	QByteArray ba= file.readAll();
	file.close();

	QJsonParseError err;
	QJsonDocument doc= QJsonDocument::fromJson( ba, &err );
	if( err.error != QJsonParseError::NoError )
	{
		h_Console::Error( "Error parsing region header ", g_world_header_filename );
		return;
	}

	QJsonObject obj= doc.object();

	ticks= obj["ticks"].toInt();

	QJsonObject rain_data_obj= obj["rain_data"].toObject();
	rain_data.is_rain= rain_data_obj["is_rain"].toBool();
	rain_data.start_tick= rain_data_obj["start_tick"].toInt();
	rain_data.duration= rain_data_obj["duration"].toInt();
	rain_data.rand_state= rain_data_obj["rand_state"].toInt();
	rain_data.base_intensity= rain_data_obj["base_intensity"].toDouble();

	QJsonObject player_obj= obj["player"].toObject();

	player.x= player_obj["x"].toDouble();
	player.y= player_obj["y"].toDouble();
	player.z= player_obj["z"].toDouble();
	player.rotation_x= player_obj["rotation_x"].toDouble();
	player.rotation_z= player_obj["rotation_z"].toDouble();
}

void h_WorldHeader::Save( const char* world_dir ) const
{
	QJsonObject obj;

	obj["ticks"]= double(ticks);

	QJsonObject rain_data_obj;
	rain_data_obj["is_rain"]= rain_data.is_rain;
	rain_data_obj["start_tick"]= int(rain_data.start_tick);
	rain_data_obj["duration"]= int(rain_data.duration);
	rain_data_obj["rand_state"]= int(rain_data.rand_state);
	rain_data_obj["base_intensity"]= double(rain_data.base_intensity);

	obj["rain_data"]= rain_data_obj;

	QJsonObject player_obj;
	player_obj["x"]= double(player.x);
	player_obj["y"]= double(player.y);
	player_obj["z"]= double(player.z);
	player_obj["rotation_x"]= player.rotation_x;
	player_obj["rotation_z"]= player.rotation_z;

	obj["player"]= player_obj;

	QJsonDocument doc(obj);
	QByteArray ba = doc.toJson();

	QFile file( QString::fromStdString(GetFullFileName(world_dir)) );
	if( !file.open( QIODevice::WriteOnly ) )
	{
		h_Console::Error( "Can not open world header file ", g_world_header_filename, "for saving." );
		return;
	}

	file.write(ba);
	file.close();
}
