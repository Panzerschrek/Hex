#ifndef RENDERER_CPP
#define RENDERER_CPP

#include "renderer.hpp"
#include <QtOpenGL>
#include "glcorearb.h"
#include "rendering_constants.hpp"


void r_Renderer::UpdateChunk(unsigned short X,  unsigned short Y )
{
    if( frame_count == 0 )
        return;
    else
        chunk_info[ X + Y * world->ChunkNumberX() ].chunk_data_updated= true;
}

void r_Renderer::UpdateChunkWater(unsigned short X,  unsigned short Y )
{
    if( frame_count == 0 )
        return;
    else
        chunk_info[ X + Y * world->ChunkNumberX() ].chunk_water_data_updated= true;
}

void r_Renderer::FullUpdate()
{
    if( frame_count == 0 )
        return;
    for( unsigned int i= 0; i< world->ChunkNumberX(); i++ )
        for( unsigned int j= 0; j< world->ChunkNumberY(); j++ )
        {
            r_ChunkInfo* ch= &chunk_info[ i + j * world->ChunkNumberX() ];
            ch->chunk_data_updated= true;
            ch->chunk_water_data_updated= true;
            ch->chunk= world->GetChunk( i, j );
            if( j!= 0 )
            {
                ch->chunk_back= world->GetChunk( i, j-1 );
                if( i!= world->ChunkNumberX() - 1 )
                    ch->chunk_back_right= world->GetChunk( i + 1, j - 1 );
                else
                    ch->chunk_back_right= NULL;
            }
            else
                ch->chunk_back_right= ch->chunk_back= NULL;

            if( j!= world->ChunkNumberY() - 1 )
                ch->chunk_front= world->GetChunk( i, j+1 );
            else
                ch->chunk_front= NULL;

            if( i!= world->ChunkNumberX() - 1 )
                ch->chunk_right= world->GetChunk( i+1, j );
            else
                ch->chunk_right= NULL;


            /*if( i < world->ChunkNumberX() - 1 )
                chunk_info[k].chunk_right= world->GetChunk( i + 1, j );
            if( j< world->ChunkNumberY() - 1 )
                chunk_info[k].chunk_front= world->GetChunk( i, j + 1 );
            if( j > 0 )
                chunk_info[k].chunk_back= world->GetChunk( i, j - 1 );
            if( j > 0 && i < world->ChunkNumberX() - 1 )
                chunk_info[k].chunk_back_right= world->GetChunk( i + 1, j - 1 );*/
        }
}

void r_Renderer::UpdateWorld()
{
    host_data_mutex.lock();
    world->Lock();

    unsigned int i, j, n;
    bool full_update= false;
    bool any_vbo_updated= false;
    r_WorldVertex *old_chunk_vb_data;
    unsigned int vb_shift;
    unsigned int new_allocated_vertex_count= 0;

    for( i= 0; i< world->ChunkNumberX(); i++ )
        for( j= 0; j< world->ChunkNumberY(); j++ )
        {
            n=  i + j * world->ChunkNumberX();
            if( chunk_info[n].chunk_data_updated )
            {
                chunk_info[n].GetQuadCount();
                if( chunk_info[n].chunk_vb.new_vertex_count > chunk_info[n].chunk_vb.allocated_vertex_count )
                    full_update= true;
            }
        }

    if( full_update )
    {
        printf( "full update\n" );
        for( i= 0; i< world->ChunkNumberX(); i++ )
            for( j= 0; j< world->ChunkNumberY(); j++ )
            {
                n=  i + j * world->ChunkNumberX();
                if( chunk_info[n].chunk_data_updated )
                    new_allocated_vertex_count+=
                        chunk_info[n].chunk_vb.allocated_vertex_count= chunk_info[n].chunk_vb.new_vertex_count +
                                ((chunk_info[n].chunk_vb.new_vertex_count >> 2 )& 0xFFFFFFFC );
                else
                    new_allocated_vertex_count+=
                        chunk_info[n].chunk_vb.allocated_vertex_count= chunk_info[n].chunk_vb.real_vertex_count +
                                ((chunk_info[n].chunk_vb.real_vertex_count >> 2)& 0xFFFFFFFC );
            }

        world_vb.new_vb_data= new r_WorldVertex[ new_allocated_vertex_count ];

        vb_shift= 0;
        //water_vb_shift= 0;
        for( i= 0; i< world->ChunkNumberX(); i++ )
            for( j= 0; j< world->ChunkNumberY(); j++ )
            {
                n=  i + j * world->ChunkNumberX();
                old_chunk_vb_data= chunk_info[n].chunk_vb.vb_data;
                chunk_info[n].chunk_vb.vb_data= world_vb.new_vb_data + vb_shift;

                chunk_info[n].chunk_vb.water_vb_data= chunk_info[n].chunk_vb.vb_data +
                                                      chunk_info[n].chunk_vb.real_vertex_count -
                                                      chunk_info[n].chunk_vb.water_vertex_count;


                if( !chunk_info[n].chunk_data_updated )
                {
                    //copy old chunk mesh data to new data array
                    memcpy( chunk_info[n].chunk_vb.vb_data, old_chunk_vb_data,
                            sizeof(r_WorldVertex) * chunk_info[n].chunk_vb.real_vertex_count );
                }
                vb_shift+= chunk_info[n].chunk_vb.allocated_vertex_count;

            }
        // world_vb.need_update_vbo= true;
    }//if full update


    for( i= 0; i< world->ChunkNumberX(); i++ )
        for( j= 0; j< world->ChunkNumberY(); j++ )
        {
            n=  i + j * world->ChunkNumberX();
            if( chunk_info[n].chunk_data_updated )
            {
                chunk_info[n].BuildChunkMesh();
                chunk_info[n].chunk_data_updated= false;
                chunk_info[n].chunk_mesh_rebuilded= true;
                any_vbo_updated= true;
                chunk_rebuild_in_last_second++;
            }
        }


    world->Unlock();
    host_data_mutex.unlock();

    if( any_vbo_updated )
    {
        gpu_data_mutex.lock();
        if( full_update )
        {
            delete[] world_vb.vb_data;
            //world_vb.vb_data= world_vb.new_vb_data;
            world_vb.allocated_vertex_count= new_allocated_vertex_count;
            world_vb.need_update_vbo= true;
        }

        world_vb.vbo_update_ready= true;
        gpu_data_mutex.unlock();
    }
}

void r_Renderer::UpdateWater()
{
    host_data_mutex.lock();
    world->Lock();

    unsigned int i, j;
    bool any_vbo_updated= false;
    bool full_update= false;

    // r_WaterVertex* new_vb_data;
    unsigned int new_vertex_count;

    //build water meshes for CHUNKS
    for( i= 0; i< chunk_num_x; i++ )
        for( j= 0 ; j< chunk_num_y; j++ )
        {
            r_ChunkInfo* ch= &chunk_info[ i + j * chunk_num_x ];
            if( ch->chunk_water_data_updated )
                ch->BuildWaterSurfaceMesh();
        }

    //calculate new vertex count for QUADchunks
    for( i= 0; i< quadchunk_num_x; i++ )
        for( j= 0; j< quadchunk_num_y; j++ )
        {
            r_WaterQuadChunkInfo* ch= &water_quadchunk_info[ i + j * quadchunk_num_x ];
            ch->GetVertexCount();
            ch->GetUpdatedState();
            if( ch->new_vertex_count > ch->allocated_vertex_count )
                full_update= true;
        }

    //reset update state of water data for CHUNKS
    for( i= 0; i< chunk_num_x; i++ )
        for( j= 0 ; j< chunk_num_y; j++ )
        {
            r_ChunkInfo* ch= &chunk_info[ i + j * chunk_num_x ];
            ch->chunk_water_data_updated= false;
        }

    if( full_update )
    {
        //calculate size of new VBO
        new_vertex_count= 0;
        for( i= 0; i< quadchunk_num_x; i++ )
            for( j= 0; j< quadchunk_num_y; j++ )
            {
                r_WaterQuadChunkInfo* ch= &water_quadchunk_info[ i + j * quadchunk_num_x ];
                if( ch->water_updated )
                    ch->allocated_vertex_count= ch->new_vertex_count + ( ch->new_vertex_count >> 2 );// +25%
                else
                    ch->allocated_vertex_count= ch->real_vertex_count + ( ch->real_vertex_count >> 2 );// +25%
                new_vertex_count+= ch->allocated_vertex_count;
            }

        //copy old unmodifed quadchunk water data to new VBO and change r_WaterQuadChunkInfo::vb_data to new
        water_vb.new_vb_data= new r_WaterVertex[ new_vertex_count ];
        r_WaterVertex* new_quadchunk_vb_data= water_vb.new_vb_data;
        for( i= 0; i< quadchunk_num_x; i++ )
            for( j= 0; j< quadchunk_num_y; j++ )
            {
                r_WaterQuadChunkInfo* ch= &water_quadchunk_info[ i + j * quadchunk_num_x ];
                r_WaterVertex* old_vb_data= ch->vb_data;
                ch->vb_data= new_quadchunk_vb_data;


                if( ! ch->water_updated )
                {
                    memcpy( ch->vb_data, old_vb_data,
                            sizeof(r_WaterVertex ) * ch->real_vertex_count );
                }
                new_quadchunk_vb_data+= ch->allocated_vertex_count;
            }
    }// if( full_update )

    for( i= 0; i< quadchunk_num_x; i++ )
        for( j= 0; j< quadchunk_num_y; j++ )
        {
            r_WaterQuadChunkInfo* ch= &water_quadchunk_info[ i + j * quadchunk_num_x ];
            if( ch->water_updated )
            {
                ch->BuildFinalMesh();
                ch->water_updated= false;
                ch->water_mesh_rebuilded= true;
                any_vbo_updated= true;
                water_quadchunks_rebuild_in_last_second++;
            }
        }

    world->Unlock();
    host_data_mutex.unlock();

    if( any_vbo_updated )
    {
        gpu_data_mutex.lock();
        water_vb.vbo_update_ready= true;
        if( full_update )
        {
            delete[] water_vb.vb_data;
            //water_vb.vb_data= water_vb.new_vb_data;
            water_vb.need_update_vbo= true;
            water_vb.allocated_vertex_count= new_vertex_count;
        }
        gpu_data_mutex.unlock();
    }

}

void r_Renderer::UpdateFunc()
{
    while(1)
    {
        QTime t0= QTime::currentTime();

        UpdateWorld();
        if( update_count&1 )
            UpdateWater();

        QTime t1= QTime::currentTime();
        unsigned int dt_ms= t0.msecsTo( t1 );
        if( dt_ms < 50 )
            usleep( (50 - dt_ms) * 1000);

        update_count++;
        update_ticks_in_last_second++;

    }
}


void r_Renderer::UpdateGPUData()
{
    gpu_data_mutex.lock();
    if( world_vb.vbo_update_ready )
    {
        host_data_mutex.lock();

        unsigned int i, j, n;

        if( world_vb.need_update_vbo )
        {
            world_vb.vb_data= world_vb.new_vb_data;
            world_vb.vbo.VertexData( (float*) world_vb.vb_data, world_vb.allocated_vertex_count * sizeof( r_WorldVertex ),
                                     sizeof( r_WorldVertex ) );
            for( i= 0; i< world->ChunkNumberX(); i++ )
                for( j= 0; j< world->ChunkNumberY(); j++ )
                {
                    n=  i + j * world->ChunkNumberX();
                    if( chunk_info[n].chunk_mesh_rebuilded )
                    {
                        chunk_info[n].chunk_mesh_rebuilded= false;
                        chunk_info[n].chunk_vb.real_vertex_count= chunk_info[n].chunk_vb.new_vertex_count;
                    }
                }

            memcpy( chunk_info_to_draw, chunk_info,
                    sizeof( r_ChunkInfo ) * world->ChunkNumberX() * world->ChunkNumberY() );

            world_vb.need_update_vbo= false;
            chunk_updates_in_last_second+= world->ChunkNumberX() * world->ChunkNumberY();
            printf( "GPU full update\n" );
        }
        else
            for( i= 0; i< world->ChunkNumberX(); i++ )
                for( j= 0; j< world->ChunkNumberY(); j++ )
                {
                    n=  i + j * world->ChunkNumberX();
                    if( chunk_info[n].chunk_mesh_rebuilded )
                    {
                        world_vb.vbo.VertexSubData( chunk_info[n].chunk_vb.vb_data,
                                                    chunk_info[n].chunk_vb.new_vertex_count * sizeof( r_WorldVertex ),
                                                    (unsigned int)( chunk_info[n].chunk_vb.vb_data - world_vb.vb_data ) * sizeof( r_WorldVertex ) );

                        chunk_info[n].chunk_mesh_rebuilded= false;
                        chunk_info[n].chunk_vb.real_vertex_count= chunk_info[n].chunk_vb.new_vertex_count;

                        memcpy( &chunk_info_to_draw[n], &chunk_info[n], sizeof( r_ChunkInfo ) );
                        chunk_updates_in_last_second++;
                    }
                }

        host_data_mutex.unlock();

        world_vb.vbo_update_ready = false;
    }//if( world_vb.vbo_update_ready )
    gpu_data_mutex.unlock();


//water update

    gpu_data_mutex.lock();
    if( water_vb.vbo_update_ready )
    {
        host_data_mutex.lock();

        if( water_vb.need_update_vbo )
        {
            water_vb.vb_data= water_vb.new_vb_data;
            water_vb.vbo.VertexData( water_vb.vb_data,
                                     water_vb.allocated_vertex_count * sizeof( r_WaterVertex ),
                                     sizeof( r_WaterVertex ) );
            for( unsigned int i= 0; i< quadchunk_num_x; i++ )
                for( unsigned int j= 0; j< quadchunk_num_y; j++ )
                {
                    r_WaterQuadChunkInfo* ch= & water_quadchunk_info[ i + j * quadchunk_num_x ];
                    if( ch->water_mesh_rebuilded )
                    {
                        ch->water_mesh_rebuilded= false;
                        ch->real_vertex_count= ch->new_vertex_count;
                    }
                }

            water_vb.need_update_vbo= false;

            memcpy( water_quadchunk_info_to_draw, water_quadchunk_info,
                    sizeof( r_WaterQuadChunkInfo ) * quadchunk_num_x * quadchunk_num_y );

            water_quadchunks_updates_in_last_second+= quadchunk_num_x * quadchunk_num_y;
            printf( "GPU water full update\n" );
        }//if( water_vb.need_update_vbo )
        else
        {
            for( unsigned int i= 0; i< quadchunk_num_x; i++ )
                for( unsigned int j= 0; j< quadchunk_num_y; j++ )
                {
                    r_WaterQuadChunkInfo* ch= & water_quadchunk_info[ i + j * quadchunk_num_x ];
                    if( ch->water_mesh_rebuilded )
                    {
                        water_vb.vbo.VertexSubData( ch->vb_data,
                                                    ch->new_vertex_count * sizeof( r_WaterVertex ),
                                                    (ch->vb_data - water_vb.vb_data) * sizeof( r_WaterVertex ) );

                        ch->water_mesh_rebuilded= false;
                        ch->real_vertex_count= ch->new_vertex_count;
                        memcpy( & water_quadchunk_info_to_draw[ i + j * quadchunk_num_x ],
                                & water_quadchunk_info[ i + j * quadchunk_num_x ],
                                sizeof( r_WaterQuadChunkInfo )  );

                        water_quadchunks_updates_in_last_second++;
                    }
                }
        }


        water_vb.vbo_update_ready= false;
        host_data_mutex.unlock();
    }
    gpu_data_mutex.unlock();
}

void r_Renderer::CalculateMatrices()
{
    m_Mat4 scale, translate, result, perspective, rotate_x, rotate_z, basis_change;

    m_Vec3 tr_vector= - cam_pos;
    translate.Identity();
    translate.Translate( tr_vector );

    static m_Vec3 s_vector( H_BLOCK_SCALE_VECTOR_X,
                            H_BLOCK_SCALE_VECTOR_Y
                            , H_BLOCK_SCALE_VECTOR_Z );//hexogonal prism scale vector. DO NOT TOUCH!
    scale.Identity();
    scale.Scale( s_vector );

    perspective.Identity();
    perspective.MakePerspective( float(viewport_x)/float(viewport_y), 1.57f,
                                 0.5f*(H_PLAYER_HEIGHT - H_PLAYER_EYE_LEVEL),//znear
                                 1024.0f );

    rotate_x.Identity();
    rotate_x.RotateX( -cam_ang.x );
    rotate_z.Identity();
    rotate_z.RotateZ( -cam_ang.z );

    basis_change.Identity();
    basis_change[5]= 0.0f;
    basis_change[6]= 1.0f;
    basis_change[9]= 1.0f;
    basis_change[10]= 0.0f;
    view_matrix= translate * rotate_z * rotate_x * basis_change * perspective;

    block_scale_matrix= scale;
    block_final_matrix= block_scale_matrix * view_matrix;
}

void r_Renderer::CalculateLight()
{
    lighting_data.current_sun_light= R_SUN_LIGHT_COLOR / float ( H_MAX_SUN_LIGHT * 16 );
    lighting_data.current_fire_light= R_FIRE_LIGHT_COLOR / float ( H_MAX_FIRE_LIGHT * 16 );
}

void r_Renderer::BuildChunkList()
{
    unsigned int k;
    r_ChunkInfo* ch;
    for( unsigned int i=0; i< world->ChunkNumberX(); i++ )
        for( unsigned int j= 0; j< world->ChunkNumberY(); j++ )
        {
            k= i + j * world->ChunkNumberX();
            ch= &chunk_info_to_draw[ k ];
            world_vb.chunk_meshes_index_count[k]= ( ch->chunk_vb.real_vertex_count - ch->chunk_vb.water_vertex_count ) * 6 / 4;
            world_vb.base_vertices[k]= ch->chunk_vb.vb_data - world_vb.vb_data;
            world_vb.multi_indeces[k]= NULL;

            world_vb.chunk_meshes_water_index_count[k]= ch->chunk_vb.water_vertex_count * 6 / 4;
            world_vb.base_water_vertices[k]= ch->chunk_vb.water_vb_data - world_vb.vb_data;
        }

    world_vb.chunks_to_draw= world->ChunkNumberX() * world->ChunkNumberY();
}


void r_Renderer::Draw()
{
    //usleep( 100 * 1000 );
    if( frame_count == 0 )
    {
        BuildWorld();
        InitVertexBuffers();
        update_thread.start();
    }

    UpdateGPUData();
    CalculateMatrices();
    CalculateLight();

    //glClearColor( 0.2f, 0.3f, 0.8f, 0.0f );
    ///glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


    BuildChunkList();
    DrawWorld();
    DrawSky();
    DrawSun();
    DrawWater();
    DrawBuildPrism();

    if( settings.value( "show_debug_info", false ).toBool() )
    {
        text_manager->AddMultiText( 0, 0, 1, r_Text::default_color, "fps: %d", last_fps );
        text_manager->AddMultiText( 0, 1, 1, r_Text::default_color, "chunks updated per second: %d", chunk_updates_per_second );
        //text_manager->AddMultiText( 0, 2, 1, r_Text::default_color, "chunks rebuilded per second: %d",chunks_rebuild_per_second );
        text_manager->AddMultiText( 0, 2, 1, r_Text::default_color, "water quadchunks updated per second: %d", water_quadchunks_updates_per_second );
        //text_manager->AddMultiText( 0, 4, 1, r_Text::default_color, "water quadchunks rebuilded per second: %d", water_quadchunks_rebuild_per_second );
        text_manager->AddMultiText( 0, 3, 1, r_Text::default_color, "update ticks per second: %d",updade_ticks_per_second );
        text_manager->AddMultiText( 0, 4, 1, r_Text::default_color, "cam pos: %4.1f %4.1f %4.1f",
                                    cam_pos.x, cam_pos.y, cam_pos.z );
        text_manager->Draw();
    }
    CalculateFPS();
}


void r_Renderer::CalculateFPS()
{
    frames_in_last_second++;

    if( last_fps_time.msecsTo( QTime::currentTime() ) > 1000 )
    {
        last_fps= frames_in_last_second;
        frames_in_last_second= 0;
        last_fps_time= QTime::currentTime();

        chunk_updates_per_second= chunk_updates_in_last_second;
        chunk_updates_in_last_second= 0;

        chunks_rebuild_per_second= chunk_rebuild_in_last_second;
        chunk_rebuild_in_last_second= 0;


        water_quadchunks_rebuild_per_second= water_quadchunks_rebuild_in_last_second;
        water_quadchunks_rebuild_in_last_second= 0;

        water_quadchunks_updates_per_second= water_quadchunks_updates_in_last_second;
        water_quadchunks_updates_in_last_second= 0;

        updade_ticks_per_second= update_ticks_in_last_second;
        update_ticks_in_last_second= 0;
    }
    frame_count++;
}

void r_Renderer::DrawWorld()
{
    glDisable( GL_BLEND );
    glCullFace( GL_FRONT );
    glEnable( GL_CULL_FACE );
    glEnable( GL_DEPTH_TEST );

    texture_manager.BindTextureArray( 0 );

    world_shader.Bind();
    world_shader.Uniform( "tex", 0 );
    world_shader.Uniform( "sun_vector", sun_vector );
    world_shader.Uniform( "view_matrix", block_final_matrix );

    world_shader.Uniform( "sun_light_color", lighting_data.current_sun_light );
    world_shader.Uniform( "fire_light_color", lighting_data.current_fire_light );
    world_shader.Uniform( "ambient_light_color", R_AMBIENT_LIGHT_COLOR );

    world_vb.vbo.Bind();

    glMultiDrawElementsBaseVertex( GL_TRIANGLES, world_vb.chunk_meshes_index_count, GL_UNSIGNED_SHORT,
                                   (const GLvoid**)(world_vb.multi_indeces), world_vb.chunks_to_draw,
                                   world_vb.base_vertices );

}

void r_Renderer::DrawSky()
{
    glDisable( GL_BLEND );
    glDisable( GL_CULL_FACE );
    glEnable( GL_DEPTH_TEST );

    skybox_shader.Bind();
    skybox_shader.Uniform( "cam_pos", cam_pos );
    skybox_shader.Uniform( "view_matrix", view_matrix );

    skybox_vbo.Bind();
    skybox_vbo.Show();
}

void r_Renderer::DrawSun()
{
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_BLEND );
    //glEnable( GL_POINT_SPRITE );
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable( GL_DEPTH_TEST );
#ifdef OGL21
    glEnable( GL_POINT_SPRITE );
#endif

    sun_texture.BindTexture(0);

    sun_shader.Bind();
    sun_shader.Uniform( "view_matrix", view_matrix );
    sun_shader.Uniform( "sun_vector", sun_vector );
    sun_shader.Uniform( "cam_pos", cam_pos );
    sun_shader.Uniform( "tex", 0 );

    glDrawArrays( GL_POINTS, 0, 1 );
}

void r_Renderer::DrawWater()
{
    glDisable( GL_CULL_FACE );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    water_texture.BindTexture( 0 );

    water_shader.Bind();

    m_Mat4 water_matrix;
    water_matrix.Identity();
    m_Vec3 scale( 1.0f, 1.0f, 1.0f/128.0f );
    water_matrix.Scale(scale);
    water_final_matrix= water_matrix *  block_final_matrix;
    water_shader.Uniform( "view_matrix", water_final_matrix );
    water_shader.Uniform( "tex", 0 );
    water_shader.Uniform( "time", float( startup_time.msecsTo(QTime::currentTime()) ) * 0.001f );

    water_shader.Uniform( "sun_light_color", lighting_data.current_sun_light );
    water_shader.Uniform( "fire_light_color", lighting_data.current_fire_light );
    water_shader.Uniform( "ambient_light_color", R_AMBIENT_LIGHT_COLOR );

    /*world_vb.vbo.Bind();
    #ifdef OGL21
    glMultiDrawArrays( GL_QUADS, world_vb.base_water_vertices,
                       world_vb.chunk_meshes_water_index_count, world_vb.chunks_to_draw );
    #else
    glMultiDrawElementsBaseVertex( GL_TRIANGLES, world_vb.chunk_meshes_water_index_count, GL_UNSIGNED_SHORT,
                                   (const GLvoid**)(world_vb.multi_indeces), world_vb.chunks_to_draw,
                                   world_vb.base_water_vertices );
    #endif
    */

    water_vb.vbo.Bind();
    //glDrawElementsBaseVertex( GL_TRIANGLES, 12 * 8, GL_UNSIGNED_SHORT, NULL, 0 );
    for( unsigned int i= 0; i< quadchunk_num_x; i++ )
        for( unsigned int j= 0; j< quadchunk_num_y; j++ )
        {
            r_WaterQuadChunkInfo* ch= & water_quadchunk_info_to_draw[ i + j * quadchunk_num_x ];
            glDrawElementsBaseVertex( GL_TRIANGLES, ch->real_vertex_count * 2,
                                      GL_UNSIGNED_SHORT, NULL, ch->vb_data - water_vb.vb_data );
        }
}

void r_Renderer::DrawBuildPrism()
{
    if( build_pos.z < 0.0f )
        return;
    glDisable( GL_BLEND );
    glEnable( GL_DEPTH_TEST );
    glLineWidth( 4.0f );

    build_prism_shader.Bind();
    build_prism_shader.Uniform( "view_matrix", view_matrix );

    build_prism_shader.Uniform( "build_prism_pos", build_pos );

    // m_Vec3 sh_cam_pos( cam_pos.x * H_SPACE_SCALE_VECTOR_X, cam_pos.y * H_SPACE_SCALE_VECTOR_Y
    build_prism_shader.Uniform( "cam_pos", cam_pos );

    build_prism_vbo.Bind();

    build_prism_vbo.Show();
}


void r_Renderer::BuildWorldWater()
{
    /*build quadchunk data structures*/
    quadchunk_num_x= 1 + world->ChunkNumberX()/2;
    //if( quadchunk_num_x * 2 != world->ChunkNumberX() )
    //	quadchunk_num_x++;
    quadchunk_num_y= 1 + world->ChunkNumberY()/2;
    //if( quadchunk_num_y * 2 != world->ChunkNumberY() )
    //	quadchunk_num_y++;
    water_quadchunk_info= new r_WaterQuadChunkInfo[ quadchunk_num_x * quadchunk_num_y ];
    water_quadchunk_info_to_draw= new r_WaterQuadChunkInfo[ quadchunk_num_x * quadchunk_num_y ];

    for( unsigned int i= 0; i< quadchunk_num_x; i++ )
        for( unsigned int j= 0; j< quadchunk_num_y; j++ )
        {
            r_WaterQuadChunkInfo* ch= &water_quadchunk_info[ i + j * quadchunk_num_x ];
            for( unsigned int n= 0; n< 2; n++ )
                for( unsigned int m= 0; m< 2; m++ )
                {
                    if( i*2 + n < chunk_num_x && j*2 + m < chunk_num_y )
                        ch->chunks[n][m]= & chunk_info[ i*2 + n + (j*2 + m) * chunk_num_x ];
                    else
                        ch->chunks[n][m]= NULL;
                    ch->water_updated= false;
                    ch->water_mesh_rebuilded= false;
                }
        }
    /*build quadchunk data structures*/

    for( unsigned int i=0; i< chunk_num_x; i++ )
        for( unsigned int j=0; j< chunk_num_y; j++ )
        {
            chunk_info[ i + j * chunk_num_x ].BuildWaterSurfaceMesh();
        }

    //allocate memory for vertices and build water quadchunk meshes
    unsigned int vertex_count= 0;
    for( unsigned int i= 0; i< quadchunk_num_x; i++ )
        for( unsigned int j= 0; j< quadchunk_num_y; j++ )
        {
            r_WaterQuadChunkInfo* ch= &water_quadchunk_info[ i + j * quadchunk_num_x ];
            ch->GetVertexCount();
            ch->allocated_vertex_count= ch->new_vertex_count + ( ch->new_vertex_count >>2 );//+ 25%
            vertex_count+= ch->allocated_vertex_count;
        }

    water_vb.allocated_vertex_count= vertex_count;
    water_vb.vb_data= new r_WaterVertex[ vertex_count ];
    r_WaterVertex* v= water_vb.vb_data;
    for( unsigned int i= 0; i< quadchunk_num_x; i++ )
        for( unsigned int j= 0; j< quadchunk_num_y; j++ )
        {
            r_WaterQuadChunkInfo* ch= &water_quadchunk_info[ i + j * quadchunk_num_x ];
            ch->vb_data= v;
            ch->BuildFinalMesh();
            v+= ch->allocated_vertex_count;
        }
    //allocate memory for vertices and build water quadchunk meshes


    //generate index buffer for hexagons
    water_vb.index_buffer_size= 65520;
    water_vb.vb_index_data= new unsigned short[ water_vb.index_buffer_size ];
    for( unsigned int i= 0, j= 0; i< water_vb.index_buffer_size; i+=12, j+= 6 )
    {
        water_vb.vb_index_data[i+0 ]= j+0;
        water_vb.vb_index_data[i+1 ]= j+1;
        water_vb.vb_index_data[i+2 ]= j+2;

        water_vb.vb_index_data[i+3 ]= j+2;
        water_vb.vb_index_data[i+4 ]= j+3;
        water_vb.vb_index_data[i+5 ]= j+4;

        water_vb.vb_index_data[i+6 ]= j+4;
        water_vb.vb_index_data[i+7 ]= j+5;
        water_vb.vb_index_data[i+8 ]= j+0;

        water_vb.vb_index_data[i+9 ]= j+0;
        water_vb.vb_index_data[i+10]= j+2;
        water_vb.vb_index_data[i+11]= j+4;
    }


    water_vb.vbo_update_ready= false;
    water_vb.need_update_vbo= false;
    memcpy( water_quadchunk_info_to_draw, water_quadchunk_info,
            sizeof( r_WaterQuadChunkInfo ) * quadchunk_num_x * quadchunk_num_y );
}

void r_Renderer::BuildWorld()
{
    world->Lock();

    chunk_num_x= world->ChunkNumberX();
    chunk_num_y= world->ChunkNumberY();

    chunk_info= new r_ChunkInfo[ world->ChunkNumberX() * world->ChunkNumberY() ];
    chunk_info_to_draw= new r_ChunkInfo[ world->ChunkNumberX() * world->ChunkNumberY() ];


    unsigned int k;
    for( unsigned int i=0; i< world->ChunkNumberX(); i++ )
        for( unsigned int j=0; j< world->ChunkNumberY(); j++ )
        {
            k= j * world->ChunkNumberX() + i;
            chunk_info[k].chunk= world->GetChunk( i, j );
            if( i < world->ChunkNumberX() - 1 )
                chunk_info[k].chunk_right= world->GetChunk( i + 1, j );
            else chunk_info[k].chunk_right= NULL;
            if( j< world->ChunkNumberY() - 1 )
                chunk_info[k].chunk_front= world->GetChunk( i, j + 1 );
            else chunk_info[k].chunk_front= NULL;
            if( j > 0 )
                chunk_info[k].chunk_back= world->GetChunk( i, j - 1 );
            else chunk_info[k].chunk_back= NULL;
            if( j > 0 && i < world->ChunkNumberX() - 1 )
                chunk_info[k].chunk_back_right= world->GetChunk( i + 1, j - 1 );
            else chunk_info[k].chunk_back_right= 0;

        }

    world_vb.allocated_vertex_count= 0;
    for( unsigned int i=0; i< world->ChunkNumberX(); i++ )
    {
        for( unsigned int j=0; j< world->ChunkNumberY(); j++ )
        {
            k= j * world->ChunkNumberX() + i;
            chunk_info[k].GetQuadCount();
            world_vb.allocated_vertex_count+=
                chunk_info[k].chunk_vb.allocated_vertex_count=
                    chunk_info[k].chunk_vb.new_vertex_count +
                    ((chunk_info[k].chunk_vb.new_vertex_count>>2) & 0xFFFFFFFC);
        }
    }

    printf( "quads in world: %d, quads per chunk: %d\n", world_vb.allocated_vertex_count >> 2,
            ( world_vb.allocated_vertex_count >> 2 ) / (  world->ChunkNumberX() * world->ChunkNumberY() ) );
    world_vb.vb_data= new r_WorldVertex[ world_vb.allocated_vertex_count ];
    world_vb.vb_index_data= new quint16[ world_vb.index_buffer_size= 65532 ];

    for( unsigned int x= 0, y=0; x< world_vb.index_buffer_size; x+=6, y+=4 )
    {
        world_vb.vb_index_data[x] = y;
        world_vb.vb_index_data[x + 1] = y + 1;
        world_vb.vb_index_data[x + 2] = y + 2;

        world_vb.vb_index_data[x + 3] = y;
        world_vb.vb_index_data[x + 4] = y + 2;
        world_vb.vb_index_data[x + 5] = y + 3;
    }

    unsigned int vb_shift= 0;
    for( unsigned int i=0; i< world->ChunkNumberX(); i++ )
    {
        for( unsigned int j=0; j< world->ChunkNumberY(); j++ )
        {
            chunk_info[ k= j * world->ChunkNumberX() + i ].chunk_vb.vb_data= world_vb.vb_data + vb_shift;
            vb_shift+= chunk_info[k].chunk_vb.allocated_vertex_count;
            chunk_info[k].BuildChunkMesh();
            chunk_info[k].chunk_vb.real_vertex_count= chunk_info[k].chunk_vb.new_vertex_count;

        }
    }

    world_vb.vbo_update_ready = true;


    memcpy( chunk_info_to_draw, chunk_info,
            sizeof( r_ChunkInfo ) * world->ChunkNumberX() * world->ChunkNumberY() );
    //chunk list data
    unsigned int c= world->ChunkNumberX() * world->ChunkNumberY();
    world_vb.chunk_meshes_index_count= new int[ c ];
    world_vb.base_vertices= new int[ c ];
    world_vb.multi_indeces= new int*[ c ];

    //chunk list water data
    world_vb.chunk_meshes_water_index_count= new int[c];
    world_vb.base_water_vertices= new int[ c ];


    BuildWorldWater();

    world->Unlock();

}

r_Renderer::r_Renderer( h_World* w ):
    world(w),
    host_data_mutex( QMutex::NonRecursive ), gpu_data_mutex( QMutex::NonRecursive ),
    update_thread( &r_Renderer::UpdateFunc, (r_Renderer*) this, 1u ),
    settings( "config.ini", QSettings::IniFormat ),
    frame_count(0),update_count(0),
    sun_vector( 0.7f, 0.8f, 0.6f )
{
    world_vb.need_update_vbo= false;

    connect( world, SIGNAL(ChunkUpdated( unsigned short, unsigned short )),
             this, SLOT( UpdateChunk( unsigned short, unsigned short ) ), Qt::DirectConnection );

    connect( world, SIGNAL(ChunkWaterUpdated( unsigned short, unsigned short )),
             this, SLOT( UpdateChunkWater( unsigned short, unsigned short ) ), Qt::DirectConnection );

    connect( world, SIGNAL(FullUpdate( void )),
             this, SLOT( FullUpdate( void ) ), Qt::DirectConnection );


    startup_time= QTime::currentTime();


    last_fps= 0;
    last_fps_time= QTime::currentTime();
    frames_in_last_second= 0;
    chunk_updates_in_last_second = chunk_updates_per_second= 0;
    chunks_rebuild_per_second= chunk_rebuild_in_last_second= 0;

    water_quadchunks_updates_per_second= water_quadchunks_updates_in_last_second= 0;
    water_quadchunks_rebuild_per_second= water_quadchunks_rebuild_in_last_second= 0;

    updade_ticks_per_second= update_ticks_in_last_second= 0;
}

void r_Renderer::InitGL()
{
    if( QGLContext::currentContext() == NULL )
        printf( "error, null gl context" );

    GetGLFunctions();
    glClearDepth( 1.0f );
    glDepthFunc( GL_LEQUAL );

    LoadShaders();
    InitFrameBuffers();
    LoadTextures();

    text_manager= new r_Text();
    text_manager->SetViewport( viewport_x, viewport_y );
}

void r_Renderer::LoadShaders()
{
    char define_str[128];

    if( world_shader.Load( "shaders/world_frag.glsl", "shaders/world_vert.glsl", NULL ) )
        printf( "error, wold shader not found\n" );

    world_shader.SetAttribLocation( "coord", 0 );
    world_shader.SetAttribLocation( "tex_coord", 1 );
    world_shader.SetAttribLocation( "normal", 2 );
    world_shader.SetAttribLocation( "light", 3 );
    sprintf( define_str, "TEX_SCALE_VECTOR vec3( %1.8f, %1.8f, %1.8f )",
             0.25f / float( H_MAX_TEXTURE_SCALE ),
             0.25f * sqrt(3.0f) / float( H_MAX_TEXTURE_SCALE ),
             1.0f );
    world_shader.Define( define_str );
    world_shader.MoveOnGPU();

    if( water_shader.Load( "shaders/water_frag.glsl", "shaders/water_vert.glsl", NULL ) )
        printf( "error, wold shader not found\n" );

    water_shader.SetAttribLocation( "coord", 0 );
    water_shader.SetAttribLocation( "light", 1 );
    water_shader.MoveOnGPU();

    if( build_prism_shader.Load( "shaders/build_prism_frag.glsl", "shaders/build_prism_vert.glsl", "shaders/build_prism_geom.glsl" ) )
        printf( "error, build prism shader not found\n" );
    build_prism_shader.SetAttribLocation( "coord", 0 );
    build_prism_shader.MoveOnGPU();

    if( skybox_shader.Load( "shaders/sky_frag.glsl", "shaders/sky_vert.glsl", NULL ) )
        printf( "errror, skybox shader not found\n" );
    skybox_shader.SetAttribLocation( "coord", 0 );
    skybox_shader.MoveOnGPU();


    if( sun_shader.Load( "shaders/sun_frag.glsl",  "shaders/sun_vert.glsl", NULL ) )
        printf( "errror, sun shader not found\n" );

    sprintf( define_str, "SUN_SIZE %3.0f\n", float( viewport_x ) * 0.1f );
    sun_shader.Define( define_str );
    sun_shader.MoveOnGPU();

}
void r_Renderer::InitFrameBuffers()
{
}
void r_Renderer::InitVertexBuffers()
{
    world_vb.vbo.VertexData( world_vb.vb_data,
                             sizeof( r_WorldVertex ) * world_vb.allocated_vertex_count,
                             sizeof( r_WorldVertex ) );

    world_vb.vbo.IndexData(  world_vb.vb_index_data,
                             sizeof(quint16) * world_vb.index_buffer_size,
                             GL_UNSIGNED_SHORT, GL_TRIANGLES );

    r_WorldVertex v;
    unsigned int shift;
    shift= ((char*)&v.coord[0]) - ((char*)&v );
    world_vb.vbo.VertexAttribPointer( 0, 3, GL_SHORT, false, shift );
    shift= ((char*)&v.tex_coord[0]) - ((char*)&v );
    world_vb.vbo.VertexAttribPointer( 1, 3, GL_SHORT, false, shift );
    shift= ((char*)&v.normal_id) - ((char*)&v );

    world_vb.vbo.VertexAttribPointerInt( 2, 1, GL_UNSIGNED_BYTE, shift );
    shift= ((char*)&v.light[0] ) - ((char*)&v );
    world_vb.vbo.VertexAttribPointer( 3, 2, GL_UNSIGNED_BYTE, false, shift );


    water_vb.vbo.VertexData( water_vb.vb_data,
                             sizeof( r_WaterVertex ) * water_vb.allocated_vertex_count,
                             sizeof( r_WaterVertex ) );
    water_vb.vbo.IndexData( water_vb.vb_index_data, sizeof( short ) * water_vb.index_buffer_size,
                            GL_UNSIGNED_SHORT, GL_TRIANGLES );

    r_WaterVertex wv;
    shift= ((char*)&wv.coord[0])- ((char*)&wv );
    water_vb.vbo.VertexAttribPointer( 0, 3, GL_SHORT, false, shift );
    shift= ((char*)&wv.light[0])- ((char*)&wv );
    water_vb.vbo.VertexAttribPointer( 1, 2, GL_UNSIGNED_BYTE, false, shift );

    /*
        water_side_vb.vbo.VertexData( water_side_vb.vb_data.Data(),
    								 sizeof( r_WaterVertex ) * water_side_vb.quad_count * 4,
    								sizeof( r_WaterVertex ) );
    	water_side_vb.vbo.IndexData( water_side_vb.index_data, sizeof(short) * water_side_vb.quad_count * 6,
    								GL_UNSIGNED_SHORT, GL_TRIANGLES );
    	shift= ((char*)&wv.coord[0])- ((char*)&wv );
        water_side_vb.vbo.VertexAttribPointer( 0, 3, GL_SHORT, false, shift );
        shift= ((char*)&wv.water_depth)- ((char*)&wv );
        water_side_vb.vbo.VertexAttribPointer( 1, 3, GL_UNSIGNED_BYTE, true, shift );*/


    float build_prism[]= { 0.0f, 0.0f, 0.0f,  2.0f, 0.0f, 0.0f,   3.0f, 1.0f, 0.0f,
                           2.0f, 2.0f, 0.0f,  0.0f, 2.0f, 0.0f,  -1.0f, 1.0f, 0.0f,
                           0.0f, 0.0f, 1.0f,  2.0f, 0.0f, 1.0f,   3.0f, 1.0f, 1.0f,
                           2.0f, 2.0f, 1.0f,  0.0f, 2.0f, 1.0f,  -1.0f, 1.0f, 1.0f
                         };
    unsigned short build_prism_indeces[]= { 0,1, 1,2, 2,3, 3,4,  4,5,   5,0,
                                            6,7, 7,8, 8,9, 9,10, 10,11, 11,6,
                                            0,6, 1,7, 2,8, 3,9,  4,10,  5,11
                                          };
    // 0,7, 1,8, 2,9, 3,10, 4,11, 5,6//additional lines
    // 0,3, 1,4, 2,5, 6,9, 7,10, 8,11  };

    build_prism_vbo.VertexData( build_prism, sizeof( build_prism ), 12 );
    build_prism_vbo.IndexData( (unsigned int*)(build_prism_indeces),
                               sizeof(build_prism_indeces), GL_UNSIGNED_SHORT, GL_LINES );
    build_prism_vbo.VertexAttribPointer( 0, 3, GL_FLOAT, false, 0 );



    short sky_vertices[]= { 256, 256, 256, -256, 256, 256,
                            256, -256, 256, -256, -256, 256,
                            256, 256, -256, -256, 256, -256,
                            256, -256, -256, -256, -256, -256
                          };
    quint16 sky_indeces[]= { 0, 1, 5,  0, 5, 4,
                             0, 4, 6,  0, 6, 2,
                             4, 5, 7,  4, 7, 6,//bottom
                             0, 3, 1,  0, 2, 3, //top
                             2, 7, 3,  2, 6, 7,
                             1, 3, 7,  1, 7, 5
                           };
    skybox_vbo.VertexData( sky_vertices, sizeof(short) * 8 * 3, sizeof(short) * 3 );
    skybox_vbo.IndexData( sky_indeces, sizeof(quint16) * 36, GL_UNSIGNED_SHORT, GL_TRIANGLES );
    skybox_vbo.VertexAttribPointer( 0, 3, GL_SHORT, false, 0 );
}


void r_Renderer::LoadTextures()
{
	texture_manager.SetTextureSize(
	max( min( settings.value( "texture_size", R_MAX_TEXTURE_RESOLUTION ).toInt(), R_MAX_TEXTURE_RESOLUTION ), R_MIN_TEXTURE_RESOLUTION ) );

	texture_manager.SetFiltration( settings.value( "filter_textures", false ).toBool() );

    texture_manager.LoadTextures();
    sun_texture.Load( "textures/sun.tga" );
    water_texture.Load( "textures/water2.tga" );
}
#endif//RENDERER_CPP
