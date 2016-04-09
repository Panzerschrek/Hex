#pragma once

// C++ representation of Json world header.
// Fields have some names in Json file.
struct h_WorldHeader
{
	void Load( const char* world_dir );
	void Save( const char* world_dir ) const;

	// World phys ticks. 0 - means uninitialized.
	unsigned int ticks= 0;

	struct
	{
		bool is_rain= false;
		unsigned int start_tick= 0;
		unsigned int duration= 0;
		unsigned int rand_state= 0;
		float base_intensity= 0.0f;
	} rain_data;

	struct
	{
		// Player coordinates. z= 0 - for uninitialized.
		float x= 0.0f;
		float y= 0.0f;
		float z= 0.0f;
		float rotation_x= 0.0f;
		float rotation_z= 0.0f;
	} player;
};
