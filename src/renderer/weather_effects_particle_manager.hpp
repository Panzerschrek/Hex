#pragma once

#include "panzer_ogl_lib.hpp"
#include "texture.hpp"
#include "glsl_program.hpp"
#include "polygon_buffer.hpp"

class r_WeatherEffectsParticleManager final
{
public:
	r_WeatherEffectsParticleManager( unsigned int particles_count, const m_Vec3& rain_zone_size );
	~r_WeatherEffectsParticleManager();

	void Draw(
		const m_Mat4& view_matrix, const m_Vec3& cam_pos,
		const r_Texture& heightmap_texture, const m_Mat4& heightmap_matrix,
		float particel_size_px,
		const m_Vec3& light );

private:
	m_Vec3 rain_zone_size_;

	unsigned int particles_count_;
	r_PolygonBuffer vbo_;
	r_GLSLProgram shader_;
	r_Texture particle_texture_;

	uint64_t startup_time_;
};
