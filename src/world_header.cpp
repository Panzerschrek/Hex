#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "console.hpp"
#include "world_header.hpp"

static const char g_world_header_filename[]= "world.json";

static QString GetFullFileName( const char* world_dir )
{
	return QString(world_dir) + "/" + g_world_header_filename;
}

void h_WorldHeader::Load( const char* world_dir )
{
	QFile file( GetFullFileName(world_dir) );
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
	QJsonObject player_obj= obj["player"].toObject();

	ticks= obj["ticks"].toInt();

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

	QJsonObject player_obj;
	player_obj["x"]= double(player.x);
	player_obj["y"]= double(player.y);
	player_obj["z"]= double(player.z);
	player_obj["rotation_x"]= player.rotation_x;
	player_obj["rotation_z"]= player.rotation_z;

	obj["player"]= player_obj;

	QJsonDocument doc(obj);
	QByteArray ba = doc.toJson();

	QFile file( GetFullFileName(world_dir) );
	if( !file.open( QIODevice::WriteOnly ) )
	{
		h_Console::Error( "Can not open world header file ", g_world_header_filename, "for saving." );
		return;
	}

	file.write(ba);
	file.close();
}
