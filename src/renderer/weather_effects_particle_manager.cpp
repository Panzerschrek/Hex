#include "weather_effects_particle_manager.hpp"

#include "../math_lib/rand.h"
#include "../console.hpp"

r_WeatherEffectsParticleManager::r_WeatherEffectsParticleManager()
	: particles_count_(0)
{

}

r_WeatherEffectsParticleManager::~r_WeatherEffectsParticleManager()
{

}


void r_WeatherEffectsParticleManager::Create( unsigned int particles_count, float rain_zone_size )
{
	particles_count_= particles_count;
	rain_zone_size_= rain_zone_size;

	{
		float* particle_coords= new float[ particles_count_ * 4 ];

		m_Rand randomizer;
		float inv_max_rand_f= 1.0f / float(randomizer.max_rand);
		for( unsigned int i= 0; i< particles_count_; i++ )
		{
			particle_coords[ i * 4     ]= float( randomizer() ) * ( rain_zone_size_ * inv_max_rand_f );
			particle_coords[ i * 4 + 1 ]= float( randomizer() ) * ( rain_zone_size_ * inv_max_rand_f );
			particle_coords[ i * 4 + 2 ]= float( randomizer() ) * ( rain_zone_size_ * inv_max_rand_f );
			particle_coords[ i * 4 + 3 ]= 1.0f;
		}

		vbo_.VertexData( particle_coords, sizeof(float)* 4 * particles_count_, sizeof(float) * 4 );
		vbo_.VertexAttribPointer( 0, 4, GL_FLOAT, false, 0 );

		delete[] particle_coords;
	}

	if( shader_.Load( "shaders/rain_frag.glsl", "shaders/rain_vert.glsl" ) )
		 h_Console::Error( "particles shader not found" );
	shader_.SetAttribLocation( "coord", 0 );

	shader_.MoveOnGPU();
}

void r_WeatherEffectsParticleManager::Draw( const m_Mat4& view_matrix, const m_Vec3& cam_pos )
{
	vbo_.Bind();

	shader_.Bind();
	shader_.Uniform( "time", float(clock())/1000.0f );
	{
		m_Vec3 particle_zone_coord= cam_pos - m_Vec3( rain_zone_size_ * 0.5f, rain_zone_size_ * 0.5f, rain_zone_size_ * 0.5f );
		shader_.Uniform( "particle_zone_coord", particle_zone_coord );
	}
	{
		m_Vec3 particle_zone_size( rain_zone_size_, rain_zone_size_, rain_zone_size_ );
		shader_.Uniform( "particle_zone_size", particle_zone_size );
	}
	shader_.Uniform( "mat", view_matrix );


	glDrawArrays( GL_POINTS, 0, particles_count_ );
}

/*r_WeatherEffectsParticleManager::r_WeatherEffectsParticleManager( unsigned int particle_num ):
    rain_zone_size(48.0f),
    use_geometry_shader( true )
{
    this->particle_num= particle_num;
    particle_coords= new float[ particle_num * 4 ];

    for( unsigned int i= 0; i< particle_num; i++ )
    {
        particle_coords[ i * 4     ]= float( rand() ) * ( rain_zone_size / 32768.0f );
        particle_coords[ i * 4 + 1 ]= float( rand() ) * ( rain_zone_size / 32768.0f );
        particle_coords[ i * 4 + 2 ]= float( rand() ) * ( rain_zone_size / 32768.0f );
        particle_coords[ i * 4 + 3 ]= 1.0f;
    }
}


void r_WeatherEffectsParticleManager::Initialize()
{
    if( use_geometry_shader || OGL21_BOOL )
    {
		#ifdef OGL21
		if( rain_shader.Load( "shaders/glsl_120/rain_frag.glsl", "shaders/glsl_120/rain_vert.glsl",
                              NULL  ) )
            printf( "error, rain shader not found\n" );
		#else
        if( rain_shader.Load( "shaders/rain/rain_frag.glsl", "shaders/rain/rain_g_vert.glsl",
                              "shaders/rain/rain_g_geom.glsl") )
            printf( "error, rain shader not found\n" );
		#endif

        rain_shader.SetAttribLocation( "coord", 0 );
        rain_shader.MoveOnGPU();

        rain_shader.FindUniform( "time" );
        rain_shader.FindUniform( "rain_velocity" );
        rain_shader.FindUniform( "particle_zone_size" );
        rain_shader.FindUniform( "particle_zone_coord" );
        rain_shader.FindUniform( "particle_size" );
        rain_shader.FindUniform( "proj_mat" );

        #ifndef OLG21
        rain_shader.FindUniform( "shadow_map" );
        rain_shader.FindUniform( "shadow_mat" );
        #endif
    }
    else
    {
        glGenBuffers( 1, &tex_buf_buffer_id );
        glBindBuffer( GL_TEXTURE_BUFFER, tex_buf_buffer_id );
        glBufferData( GL_TEXTURE_BUFFER, particle_num * 4 * sizeof(float), particle_coords, GL_STATIC_DRAW );
        delete[] 	particle_coords;

        glGenTextures( 1, &tex_buf_texture_id );
        glBindTexture( GL_TEXTURE_BUFFER, tex_buf_texture_id );
        glTexBuffer( GL_TEXTURE_BUFFER, GL_RGBA32F, tex_buf_buffer_id );


        if( rain_shader.Load( "shaders/rain/rain_frag.glsl", "shaders/rain/rain_vert.glsl", NULL ) )
            printf( "error, rain shader not found\n" );

        rain_shader.SetAttribLocation( "coord", 0 );
        rain_shader.MoveOnGPU();

        rain_shader.FindUniform( "time" );
        rain_shader.FindUniform( "rain_velocity" );
        rain_shader.FindUniform( "particle_zone_size" );
        rain_shader.FindUniform( "particle_zone_coord" );
        rain_shader.FindUniform( "particle_coords" );
        rain_shader.FindUniform( "particle_size" );
        rain_shader.FindUniform( "proj_mat" );
        rain_shader.FindUniform( "shadow_map" );
        rain_shader.FindUniform( "shadow_mat" );
    }
    rain_texture.Load( "textures/rain.tga" );
    snow_texture.Load( "textures/snow.tga" );
}


void r_WeatherEffectsParticleManager::InitVertexBuffer()
{
    if( use_geometry_shader || OGL21_BOOL )
    {
        particles_vbo.VertexData( particle_coords, particle_num * 4 * sizeof(float), 4 * sizeof(float), 0 );
        delete[] 	particle_coords;
        particles_vbo.IndexData( NULL, 64, GL_UNSIGNED_SHORT, GL_POINTS );//hack
        particles_vbo.SetPrimitiveType( GL_POINTS );
        particles_vbo.VertexAttribPointer( 0, 4, GL_FLOAT, false, 0 );
    }
}
void r_WeatherEffectsParticleManager::ShowWeatherParticles( r_WeatherEffects effect )
{
    if( effect == RAIN )
        rain_texture.BindTexture( 0 );
    else if ( effect == SNOW )
        snow_texture.BindTexture( 0 );

	shadow_map->BindDepthTexture( 1 );

    rain_shader.Bind();
    rain_shader.Uniform( "time", float( clock() ) / float( CLOCKS_PER_SEC ) );
    rain_shader.Uniform( "proj_mat", view_matrix );
    rain_shader.Uniform( "particle_zone_coord", particle_zone_coord );
    rain_shader.Uniform( "particle_zone_size", rain_zone_size );

	#ifndef OGL21 //ogl21 has no shadows
	rain_shader.Uniform( "shadow_map", 1 );
	rain_shader.Uniform( "shadow_mat", shadow_matrix );
	#endif

    if( effect == RAIN )
    	rain_shader.Uniform( "rain_velocity", RAIN_VELOCITY );
	else if ( effect == SNOW )
		rain_shader.Uniform( "rain_velocity", SNOW_VELOCITY );

    rain_shader.Uniform( "particle_size", particle_size );

    rain_shader.Uniform( "tex", 0 );

    if( use_geometry_shader || OGL21_BOOL )
    {
    	#ifdef OGL21
    		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE );
    		glEnable( GL_POINT_SPRITE );
    	#endif

        particles_vbo.Bind();
        glDrawArrays( GL_POINTS, 0, particle_num );//hack, don`t work without index buffer
    }
    else
    {
        glActiveTexture( GL_TEXTURE2 );
        glBindTexture( GL_TEXTURE_BUFFER, tex_buf_texture_id );
        rain_shader.Uniform( "particle_coords", 2 );

        glDrawElementsInstanced( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL, particle_num );
    }
}*/

