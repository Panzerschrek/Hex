#ifndef WEATHER_EFFECTS_PARTICLE_MANAGER_HPP
#define WEATHER_EFFECTS_PARTICLE_MANAGER_HPP

#include "ph.h"
#include "framebuffer_texture.hpp"
#include "glsl_program.hpp"
#include "polygon_buffer.hpp"


class r_WeatherEffectsParticleManager
{

public:
	r_WeatherEffectsParticleManager();
	~r_WeatherEffectsParticleManager();
	void Create( unsigned int particles_count, const m_Vec3& rain_zone_size );
	void Destroy();

	void Draw( const m_Mat4& view_matrix, const m_Vec3& cam_pos );

private:

	m_Vec3 rain_zone_size_;

	unsigned int particles_count_;
	r_PolygonBuffer vbo_;
	r_GLSLProgram shader_;
	r_FramebufferTexture particle_texture_;
};


#endif//WEATHER_EFFECTS_PARTICLE_MANAGER_HPP
