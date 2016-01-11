#include <limits>
#include <time.h>

#include "weather_effects_particle_manager.hpp"

#include "img_utils.hpp"
#include "../math_lib/rand.hpp"
#include "../console.hpp"
#include "../ticks_counter.hpp"
#include "ogl_state_manager.hpp"

r_WeatherEffectsParticleManager::r_WeatherEffectsParticleManager()
	: particles_count_(0)
	, startup_time_(hGetTimeMS())
{
}

r_WeatherEffectsParticleManager::~r_WeatherEffectsParticleManager()
{
}

void r_WeatherEffectsParticleManager::Create( unsigned int particles_count, const m_Vec3& rain_zone_size )
{
	particles_count_= particles_count;
	rain_zone_size_= rain_zone_size;

	{
		std::vector<unsigned short> particle_coords( particles_count_ * 4 );

		m_Rand randomizer;
		int max_ushort= std::numeric_limits<unsigned short>::max();
		for( unsigned int i= 0; i< particles_count_; i++ )
		{
			particle_coords[ i * 4     ]= randomizer.RandI( max_ushort );
			particle_coords[ i * 4 + 1 ]= randomizer.RandI( max_ushort );
			particle_coords[ i * 4 + 2 ]= randomizer.RandI( max_ushort );
			particle_coords[ i * 4 + 3 ]= max_ushort;
		}

		vbo_.VertexData( particle_coords.data(), sizeof(unsigned short) * 4 * particles_count_, sizeof(unsigned short) * 4 );
		vbo_.VertexAttribPointer( 0, 4, GL_UNSIGNED_SHORT, true, 0 );
	}

	if( !shader_.Load( "shaders/rain_frag.glsl", "shaders/rain_vert.glsl" ) )
		h_Console::Error( "particles shader not found" );
	shader_.SetAttribLocation( "coord", 0 );

	shader_.Create();

	r_ImgUtils::LoadTexture( &particle_texture_, "textures/rain_particle.png" );
}

void r_WeatherEffectsParticleManager::Destroy()
{
}

void r_WeatherEffectsParticleManager::Draw( const m_Mat4& view_matrix, const m_Vec3& cam_pos )
{
	static const GLenum blend_func[2]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
	static const r_OGLState state(
		true, false, true, true,
		blend_func );
	r_OGLStateManager::UpdateState( state );

	vbo_.Bind();

	particle_texture_.Bind(0);

	shader_.Bind();
	shader_.Uniform( "particle_coord_delta", float(hGetTimeMS() - startup_time_) * 0.001f * m_Vec3( 0.0f, 0.0f, -2.8f ) );
	shader_.Uniform( "particle_zone_coord", cam_pos - rain_zone_size_ * 0.5f );
	shader_.Uniform( "particle_zone_size", rain_zone_size_ );
	shader_.Uniform( "mat", view_matrix );
	shader_.Uniform( "tex", 0 );

	glDrawArrays( GL_POINTS, 0, particles_count_ );
}


