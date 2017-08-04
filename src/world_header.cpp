#include <fstream>

#include <PanzerJson/parser.hpp>
#include <PanzerJson/streamed_serializer.hpp>

#include "console.hpp"
#include "world_header.hpp"

static const char g_world_header_filename[]= "world.json";

static std::string GetFullFileName( const char* const world_dir )
{
	return std::string(world_dir) + "/" + g_world_header_filename;
}

void h_WorldHeader::Load( const char* const world_dir )
{
	const std::string header_path= GetFullFileName( world_dir );
	std::FILE* const f= std::fopen( header_path.c_str() , "rb" );
	if( f == nullptr )
	{
		h_Console::Error( "Can not open world header file ", header_path );
		return;
	}

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

	const PanzerJson::Value obj= parse_result->root;

	ticks= obj["ticks"].AsUint();

	const PanzerJson::Value rain_data_obj= obj["rain_data"];
	rain_data.is_rain= rain_data_obj["is_rain"].AsInt() != 0;
	rain_data.start_tick= rain_data_obj["start_tick"].AsUint();
	rain_data.duration= rain_data_obj["duration"].AsUint();
	rain_data.rand_state= rain_data_obj["rand_state"].AsUint();
	rain_data.base_intensity= rain_data_obj["base_intensity"].AsFloat();

	const PanzerJson::Value player_obj= obj["player"];
	player.x= player_obj["x"].AsFloat();
	player.y= player_obj["y"].AsFloat();
	player.z= player_obj["z"].AsFloat();
	player.rotation_x= player_obj["rotation_x"].AsFloat();
	player.rotation_z= player_obj["rotation_z"].AsFloat();
}

void h_WorldHeader::Save( const char* const world_dir ) const
{
	std::ofstream stream( GetFullFileName( world_dir ) );

	PanzerJson::StreamedSerializer<std::ofstream> serializer( stream );
	auto obj= serializer.AddObject();
	obj.AddNumber( "ticks", ticks );
	{
		auto raind_data_obj= obj.AddObject( "rain_data" );
		raind_data_obj.AddBool( "is_rain", rain_data.is_rain );
		raind_data_obj.AddNumber( "start_tick", rain_data.is_rain );
		raind_data_obj.AddNumber( "duration", rain_data.duration );
		raind_data_obj.AddNumber( "rand_state", rain_data.rand_state );
		raind_data_obj.AddNumber( "base_intensity", rain_data.base_intensity );
	}
	{
		auto player_obj= obj.AddObject( "player" );
		player_obj.AddNumber( "x", player.x );
		player_obj.AddNumber( "y", player.y );
		player_obj.AddNumber( "z", player.z );
		player_obj.AddNumber( "rotation_x", player.rotation_x );
		player_obj.AddNumber( "rotation_z", player.rotation_z );
	}
}
