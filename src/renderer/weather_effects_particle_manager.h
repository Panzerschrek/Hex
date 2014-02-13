	/*
	*This file is part of FREG.
	*
	*FREG is free software: you can redistribute it and/or modify
	*it under the terms of the GNU General Public License as published by
	*the Free Software Foundation, either version 3 of the License, or
	*(at your option) any later version.
	*
	*FREG is distributed in the hope that it will be useful,
	*but WITHOUT ANY WARRANTY; without even the implied warranty of
	*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	*GNU General Public License for more details.
	*
	*You should have received a copy of the GNU General Public License
	*along with FREG. If not, see <http://www.gnu.org/licenses/>.*/
#ifndef WEATHER_EFFECTS_PARTICLE_MANAGER_H
#define WEATHER_EFFECTS_PARTICLE_MANAGER_H

#include "ph.h"
#include "texture.h"
#include "glsl_program.h"
#include "polygon_buffer.h"
#include "frame_buffer.h"

#define RAIN_VELOCITY 20.0f
#define SNOW_VELOCITY 2.0f

class r_WeatherEffectsParticleManager
{
	public:

	enum r_WeatherEffects
	{
		RAIN,
		SNOW
	};

	void SetViewMatrix( m_Mat4& mat );
	void SetCamPosition( m_Vec3& pos );
	void SetParticleSize( m_Vec3& s );
	void SetSunShadowmap( const r_FrameBuffer* sm, m_Mat4& mat );
	void ShowWeatherParticles( r_WeatherEffects effect= RAIN );
	r_WeatherEffectsParticleManager( unsigned int particle_num );
	void UseGeometryShader( bool use );
	void Initialize();
	void InitVertexBuffer();


	private:

	unsigned int particle_num;
	r_GLSLProgram rain_shader;
	r_PolygonBuffer particles_vbo;
	r_Texture rain_texture, snow_texture;
	float* particle_coords;
	m_Vec3 particle_zone_coord, hightmap_coord;
	m_Mat4 view_matrix;
	GLuint tex_buf_texture_id, tex_buf_buffer_id;
	m_Vec3 particle_size;

	r_FrameBuffer* shadow_map;
	m_Mat4 shadow_matrix;

	bool use_geometry_shader;

	const float rain_zone_size;
};


inline void r_WeatherEffectsParticleManager::SetViewMatrix( m_Mat4& mat )
{
	view_matrix= mat;
}

inline void r_WeatherEffectsParticleManager::SetCamPosition( m_Vec3& pos )
{
	particle_zone_coord= pos - m_Vec3( 0.5f * rain_zone_size, 0.5f * rain_zone_size, 0.5f * rain_zone_size );
}

inline void r_WeatherEffectsParticleManager::UseGeometryShader( bool use )
{
	use_geometry_shader= use;
}

inline void r_WeatherEffectsParticleManager::SetParticleSize( m_Vec3& s )
{
	particle_size= s;
}

inline void r_WeatherEffectsParticleManager::SetSunShadowmap( const r_FrameBuffer* sm, m_Mat4& mat )
{
	shadow_map= (r_FrameBuffer*) sm;
	shadow_matrix= mat;
}
#endif//WEATHER_EFFECTS_PARTICLE_MANAGER_H
