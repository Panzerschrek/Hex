#ifndef WEATHER_EFFECTS_PARTICLE_MANAGER_HPP
#define WEATHER_EFFECTS_PARTICLE_MANAGER_HPP

#include "ph.h"
#include "framebuffer_texture.hpp"
#include "glsl_program.h"
#include "polygon_buffer.h"
#include "framebuffer.hpp"

#define RAIN_VELOCITY 20.0f
#define SNOW_VELOCITY 2.0f

class r_WeatherEffectsParticleManager
{
	public:


	//void SetViewMatrix( m_Mat4& mat );
	//void SetCamPosition( m_Vec3& pos );
	//void SetParticleSize( m_Vec3& s );
	//void SetSunShadowmap( const r_FrameBuffer* sm, m_Mat4& mat );
	//void ShowWeatherParticles( r_WeatherEffects effect= RAIN );
	//r_WeatherEffectsParticleManager( unsigned int particle_num );
	//void UseGeometryShader( bool use );
	//void Initialize();
	//void InitVertexBuffer();

	r_WeatherEffectsParticleManager();
	~r_WeatherEffectsParticleManager();
	void Create( unsigned int particles_count, float rain_zone_size );
	void Destroy();

	void Draw( const m_Mat4& view_matrix, const m_Vec3& cam_pos );

	private:

	float rain_zone_size_;

	unsigned int particles_count_;
	r_PolygonBuffer vbo_;
	r_GLSLProgram shader_;
};


#endif//WEATHER_EFFECTS_PARTICLE_MANAGER_HPP
