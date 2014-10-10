#ifndef WORLD_CPP
#define WORLD_CPP

#include "world.hpp"
#include "player.hpp"
#include "math_lib/m_math.h"

bool h_World::InBorders( short x, short y, short z )
{
    bool outside= x < 0 || y < 0 ||
                  x > H_CHUNK_WIDTH * ChunkNumberX() || y > H_CHUNK_WIDTH * ChunkNumberY() ||
                  z < 0 || z >= H_CHUNK_HEIGHT;
    return !outside;
}

bool h_World::CanBuild( short x, short y, short z )
{
    return GetChunk( x>> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )->
           GetBlock( x% H_CHUNK_WIDTH, y% H_CHUNK_WIDTH, z )->Type() == AIR;
}



void h_World::InitNormalBlocks()
{
    int i;
    for( i= 0; i< NUM_BLOCK_TYPES; i++ )
        new ( normal_blocks + i ) h_Block( h_BlockType(i) );
}


h_World::h_World():
	chunk_loader( "world" ),
    phys_tick_count(0),
    phys_thread( &h_World::PhysTick, this, 1u ),
    world_mutex( QMutex::NonRecursive ),
    player( NULL ),
    settings( "config.ini", QSettings::IniFormat )
{
    InitNormalBlocks();

    chunk_number_x= max( min( settings.value( QString( "chunk_number_x" ), 14 ).toInt(), H_MAX_CHUNKS ), H_MIN_CHUNKS );
    chunk_number_y= max( min( settings.value( QString( "chunk_number_y" ), 12 ).toInt(), H_MAX_CHUNKS ), H_MIN_CHUNKS ); ;
    longitude= -(chunk_number_x/2);
    latitude= -(chunk_number_y/2);
    //chunk_matrix_size_x_log2= m_Math::NearestPOTLog2( chunk_number_x );
    //chunk_matrix_size_x= 1 << chunk_matrix_size_x_log2;

   // chunks= new h_Chunk*[ chunk_matrix_size_x * chunk_number_y ];
    // for( int i=0; i< chunk_number_x * chunk_number_y; i++ )
    //    chunks[i]= new h_Chunk( this, (i%chunk_number_x) + longitude, (i/chunk_number_x) + latitude );
    for( unsigned int i= 0; i< chunk_number_x; i++ )
        for( unsigned int j= 0; j< chunk_number_y; j++ )
        {
            chunks[ i + j * H_MAX_CHUNKS ]= LoadChunk( i+longitude, j+latitude);
            //new h_Chunk( this, i + longitude, j + latitude );
        }

    LightWorld();

	phys_thread.setStackSize( 16 * 1024 * 1024 );//inrease stack for recursive methods( lighting, blasts, etc )
    phys_thread.start();
}


void h_World::Lock()
{
    world_mutex.lock();
}

void h_World::Unlock()
{
    world_mutex.unlock();
}


void h_World::BuildPhysMesh( h_ChunkPhysMesh* phys_mesh, short x_min, short x_max, short y_min, short y_max, short z_min, short z_max )
{
    short x, y, z;
    short x1, y1;
    phys_mesh->block_sides.Resize(0);
    phys_mesh->upper_block_faces.Resize(0);

    short X= Longitude() * H_CHUNK_WIDTH;
    short Y= Latitude() * H_CHUNK_WIDTH;

    x_min= max( short(2), x_min );
    y_min= max( short(2), y_min );
    z_min= max( short(0), z_min );
    x_max= min( x_max, short( chunk_number_x * H_CHUNK_WIDTH - 2 ) );
    y_max= min( y_max, short( chunk_number_y * H_CHUNK_WIDTH - 2 ) );
    z_max= min( z_max, short( H_CHUNK_HEIGHT - 1 ) );

    p_UpperBlockFace* block_face;
    p_BlockSide* block_side;

    unsigned char t, t_up, t_f, t_fr, t_br;
    for( x= x_min; x< x_max; x++ )
        for( y= y_min; y< y_max; y++ )
        {
            t_up= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )
                  ->Transparency( x&(H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), z_min );
            for( z= z_min; z < z_max; z++ )
            {
                t= t_up;

                y1= y+1;
                t_f= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y1>> H_CHUNK_WIDTH_LOG2 )
                     ->Transparency( x&(H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );

                x1= x+1;
                y1= y + ( 1&(x+1) );
                t_fr= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 )
                      ->Transparency( x1&(H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );

                x1= x+1;
                y1= y - (x&1);
                t_br= GetChunk( x1 >> H_CHUNK_WIDTH_LOG2, y1 >> H_CHUNK_WIDTH_LOG2 )
                      ->Transparency( x1&(H_CHUNK_WIDTH-1), y1&(H_CHUNK_WIDTH-1), z );

                t_up= GetChunk( x >> H_CHUNK_WIDTH_LOG2, y>> H_CHUNK_WIDTH_LOG2 )
                      ->Transparency( x&(H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), z + 1 );

                if( t != t_up )
                {
                    phys_mesh->upper_block_faces.AddToSize(1);
                    block_face=  phys_mesh->upper_block_faces.Last();
                    if( t > t_up )
                        block_face->Gen( x + X, y + Y, z + 1, DOWN );
                    else
                        block_face->Gen( x + X, y + Y, z, UP );
                }
                if( t != t_fr )
                {
                    phys_mesh->block_sides.AddToSize(1);
                    block_side= phys_mesh->block_sides.Last();
                    if( t > t_fr )
                        block_side->Gen( x + X + 1, y + Y + ((x+1)&1), z-1, BACK_LEFT );
                    else
                        block_side->Gen( x + X, y + Y, z-1, FORWARD_RIGHT );
                    \
                }
                if( t != t_br )
                {
                    phys_mesh->block_sides.AddToSize(1);
                    block_side= phys_mesh->block_sides.Last();
                    if( t > t_br )
                        block_side->Gen( x + X + 1, y + Y - (x&1), z-1, FORWARD_LEFT );
                    else
                        block_side->Gen( x + X, y + Y, z-1, BACK_RIGHT );
                }
                if( t!= t_f )
                {
                    phys_mesh->block_sides.AddToSize(1);
                    block_side= phys_mesh->block_sides.Last();
                    if( t > t_f )
                        block_side->Gen( x + X, y + Y + 1, z-1, BACK );
                    else
                        block_side->Gen( x + X, y + Y, z-1, FORWARD );
                }
            }
        }
}

bool h_World::WaterFlow( h_LiquidBlock* from, short to_x, short to_y, short to_z )
{
    h_Chunk* ch= GetChunk( to_x >> H_CHUNK_WIDTH_LOG2, to_y >> H_CHUNK_WIDTH_LOG2 );
    h_LiquidBlock* b2= (h_LiquidBlock*)ch->GetBlock( to_x & ( H_CHUNK_WIDTH-1 ), to_y & ( H_CHUNK_WIDTH-1 ), to_z );
    unsigned char type= b2->Type();
    if( type == AIR )
    {
        if( from->LiquidLevel() > 1 )
        {
            short level_delta= from->LiquidLevel() / 2;
            from->DecreaseLiquidLevel( level_delta );

            h_LiquidBlock* new_block= ch->NewWaterBlock();
            new_block->x= to_x & ( H_CHUNK_WIDTH-1 );
            new_block->y= to_y & ( H_CHUNK_WIDTH-1 );
            new_block->z= to_z;
            new_block->SetLiquidLevel( level_delta );
            ch->SetBlockAndTransparency( new_block->x, new_block->y, new_block->z , new_block, TRANSPARENCY_LIQUID );
            return true;
        }
    }
    else if( type == WATER )
    {
        short water_level_delta= from->LiquidLevel() - b2->LiquidLevel();
        if( water_level_delta  > 1 )
        {
            water_level_delta/= 2;
            from->DecreaseLiquidLevel( water_level_delta );
            b2->IncreaseLiquidLevel( water_level_delta );
            return true;
        }
    }
    return false;
}

void h_World::WaterPhysTick()
{
    h_Chunk* ch;
    for( unsigned int i= 1; i< ChunkNumberX()-1; i++ )
        for( unsigned int j= 1; j< ChunkNumberY()-1; j++ )
        {
            bool chunk_modifed= false;
            ch= GetChunk( i, j );

			//skip some quadchunks for updating ( in chessboard order )
            if( ( ( ChunkCoordToQuadchunkX( ch->Longitude() ) ^ ChunkCoordToQuadchunkY( ch->Latitude() ) ) & 1 ) == (phys_tick_count&1) )
            	continue;

            auto l= & ch->water_blocks_data.water_block_list;
            h_LiquidBlock* b;
            m_Collection< h_LiquidBlock* >::Iterator iter(l);
            for( iter.Begin(); iter.IsValid(); iter.Next() )
            {
                b= *iter;

                if( ch->GetBlock( b->x, b->y, b->z - 1 )->Type() == AIR )
                {
                    b->z--;
                    ch->SetBlockAndTransparency( b->x, b->y, b->z, b, TRANSPARENCY_LIQUID );
                    ch->SetBlockAndTransparency( b->x, b->y, b->z+1, NormalBlock( AIR ), TRANSPARENCY_AIR );
                    chunk_modifed= true;
                }
                else
                {
                    //water flow down
                    h_LiquidBlock* b2= (h_LiquidBlock*)ch->GetBlock( b->x, b->y, b->z - 1 );
                    if( b2->Type() == WATER )
                    {
                        short max_down_cell_water_level;
                        if( b->LiquidLevel() >= H_MAX_WATER_LEVEL )
                            max_down_cell_water_level= b->LiquidLevel() + H_WATER_COMPRESSION_PER_BLOCK;
                        else
                        {
                            max_down_cell_water_level= H_MAX_WATER_LEVEL + (b->LiquidLevel() *
                                                       H_WATER_COMPRESSION_PER_BLOCK) /H_MAX_WATER_LEVEL;
                        }

                        short level_delta= max_down_cell_water_level - b2->LiquidLevel();
                        if( level_delta > 0 )
                        {
                            level_delta= min( level_delta, (short) b->LiquidLevel() );
                            b->DecreaseLiquidLevel( level_delta );
                            b2->IncreaseLiquidLevel( level_delta );
                            chunk_modifed= true;
                        }
                        else if( level_delta < 0 )
                        {
                            level_delta= min( (short)-level_delta, (short)b2->LiquidLevel() );
                            b->IncreaseLiquidLevel( level_delta );
                            b2->DecreaseLiquidLevel( level_delta );
                            chunk_modifed= true;
                        }
                    }//water flow down

                    //water flow up
                    b2= (h_LiquidBlock*)ch->GetBlock( b->x, b->y, b->z + 1 );
                    if( b2->Type() == AIR )
                    {
                        short level_delta= b->LiquidLevel() - H_MAX_WATER_LEVEL;
                        if( level_delta > 0  )
                        {
                            //short level_delta= from->LiquidLevel() / 2;
                            b->DecreaseLiquidLevel( level_delta );

                            h_LiquidBlock* new_block= ch->NewWaterBlock();
                            new_block->x= b->x;
                            new_block->y= b->y;
                            new_block->z= b->z+1;
                            new_block->SetLiquidLevel( level_delta );
                            ch->SetBlockAndTransparency( new_block->x, new_block->y, new_block->z , new_block, TRANSPARENCY_LIQUID );
                            chunk_modifed= true;
                        }
                    }//water flow up

                    if( WaterFlow( b, b->x + ( i << H_CHUNK_WIDTH_LOG2 ),
                                   b->y + ( j << H_CHUNK_WIDTH_LOG2 ) + 1, b->z ) )//forward
                        chunk_modifed= true;
                    if( WaterFlow( b, b->x + ( i << H_CHUNK_WIDTH_LOG2 ) + 1,
                                   b->y  + ( j << H_CHUNK_WIDTH_LOG2 ) + ((b->x+1)&1), b->z )  )//FORWARD_RIGHT
                        chunk_modifed= true;
                    if( WaterFlow( b, b->x + ( i << H_CHUNK_WIDTH_LOG2 ) - 1,
                                   b->y  + ( j << H_CHUNK_WIDTH_LOG2 ) + ((b->x+1)&1), b->z )  )//FORWARD_LEFT
                        chunk_modifed= true;

                    if( WaterFlow( b, b->x + ( i << H_CHUNK_WIDTH_LOG2 ),
                                   b->y  + ( j << H_CHUNK_WIDTH_LOG2 ) - 1, b->z )  )//back
                        chunk_modifed= true;
                    if( WaterFlow( b, b->x + ( i << H_CHUNK_WIDTH_LOG2 ) + 1,
                                   b->y  + ( j << H_CHUNK_WIDTH_LOG2 ) - (b->x&1), b->z )  )//BACK_RIGHT
                        chunk_modifed= true;
                    if( WaterFlow( b, b->x + ( i << H_CHUNK_WIDTH_LOG2 ) - 1,
                                   b->y  + ( j << H_CHUNK_WIDTH_LOG2 ) - (b->x&1), b->z )  )//BACK_LEFT
                        chunk_modifed= true;

                    if( b->LiquidLevel() == 0 ||
					 (  b->LiquidLevel() < 16 && ch->GetBlock( b->x, b->y, b->z-1 )->Type() != WATER ) )
                    {
                        iter.RemoveCurrent();
                        ch->SetBlockAndTransparency( b->x,
                                                     b->y, b->z, NormalBlock( AIR ), TRANSPARENCY_AIR );
                        ch->DeleteWaterBlock( b );
                    }
                }// if down block not air
            }//for all water blocks in chunk
            if( chunk_modifed )
            {
                emit ChunkWaterUpdated( i, j );
                if( i > 0 )
                    emit ChunkWaterUpdated( i-1, j );
                if( i < ChunkNumberX() - 1 );
                emit ChunkWaterUpdated( i+1, j );
                if( j > 0 )
                    emit ChunkWaterUpdated( i, j-1 );
                if( j < ChunkNumberY() - 1 );
                emit ChunkWaterUpdated( i, j+1 );

                ch->need_update_light= true;
            }
        }//for chunks
}


void h_World::PhysTick()
{
    while( player == NULL )
        usleep( 1000000 );
    QTime t0= QTime::currentTime();

    Lock();

	FlushActionQueue();
	WaterPhysTick();
	RelightWaterModifedChunksLight();


    player->Lock();
    player_coord[2]= short( player->Pos().z );
    GetHexogonCoord( player->Pos().xy(), &player_coord[0], &player_coord[1] );
    player->Unlock();

    player_coord[0]-= Longitude() * H_CHUNK_WIDTH;
    player_coord[1]-= Latitude() * H_CHUNK_WIDTH;
    BuildPhysMesh( &player_phys_mesh,
                   player_coord[0] - 4, player_coord[0] + 4,
                   player_coord[1] - 5, player_coord[1] + 5,
                   player_coord[2] - 5, player_coord[2] + 5 );



	if( player_coord[1]/H_CHUNK_WIDTH > chunk_number_y/2+2 )
		MoveWorld( NORTH );
	else if( player_coord[1]/H_CHUNK_WIDTH < chunk_number_y/2-2 )
		MoveWorld( SOUTH );
	if( player_coord[0]/H_CHUNK_WIDTH > chunk_number_x/2+2 )
		MoveWorld( EAST );
	else if( player_coord[0]/H_CHUNK_WIDTH < chunk_number_x/2-2 )
		MoveWorld( WEST );

    player->Lock();
    player->SetCollisionMesh( &player_phys_mesh );
    player->Unlock();


    phys_tick_count++;
    Unlock();
    QTime t1= QTime::currentTime();
    unsigned int dt_ms= t0.msecsTo(t1);
    if( dt_ms < 50 )
        usleep( (50 - dt_ms ) * 1000 );
}


void h_World::UpdateInRadius( short x, short y, short r )
{
	short x_min, x_max, y_min, y_max;
	x_min= ClampX( x - r );
	x_max= ClampX( x + r );
	y_min= ClampY( y - r );
	y_max= ClampY( y + r );

	x_min>>= H_CHUNK_WIDTH_LOG2;
	x_max>>= H_CHUNK_WIDTH_LOG2;
	y_min>>= H_CHUNK_WIDTH_LOG2;
	y_max>>= H_CHUNK_WIDTH_LOG2;
	for( short i= x_min; i<= x_max; i++ )
		for( short j= y_min; j<= y_max; j++ )
			 emit ChunkUpdated( i, j );

}
void h_World::UpdateWaterInRadius( short x, short y, short r )
{
	short x_min, x_max, y_min, y_max;
	x_min= ClampX( x - r );
	x_max= ClampX( x + r );
	y_min= ClampY( y - r );
	y_max= ClampY( y + r );

	x_min>>= H_CHUNK_WIDTH_LOG2;
	x_max>>= H_CHUNK_WIDTH_LOG2;
	y_min>>= H_CHUNK_WIDTH_LOG2;
	y_max>>= H_CHUNK_WIDTH_LOG2;
	for( short i= x_min; i<= x_max; i++ )
		for( short j= y_min; j<= y_max; j++ )
			 emit ChunkWaterUpdated( i, j );
}

void h_World::Destroy( short x, short y, short z )
{
    if( !InBorders( x, y, z ) )
        return;

    short X= x>> H_CHUNK_WIDTH_LOG2, Y = y>> H_CHUNK_WIDTH_LOG2;
    x&= H_CHUNK_WIDTH - 1;
    y&= H_CHUNK_WIDTH - 1;

    h_Chunk* ch=GetChunk( X, Y );
    if( ch->GetBlock( x, y, z )->Type() == WATER )
    {

    }
    else if( ch->GetBlock( x, y, z )->Type() == FIRE_STONE )
    {
    	ch->DeleteLightSource( x, y, z );
    	ch->SetBlockAndTransparency( x, y, z, NormalBlock( AIR ),
                                      TRANSPARENCY_AIR );
		RelightBlockAdd( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), z );
		RelightBlockRemove( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), z );
		UpdateInRadius( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), H_MAX_FIRE_LIGHT );
		UpdateWaterInRadius( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), H_MAX_FIRE_LIGHT );
    }
    else
    {
        ch->SetBlockAndTransparency( x, y, z, NormalBlock( AIR ),
                                      TRANSPARENCY_AIR );
		RelightBlockRemove( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), z );
		//AddFireLight_r( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), z, 10 );
		UpdateInRadius( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), H_MAX_SUN_LIGHT );
		UpdateWaterInRadius( x + (X<< H_CHUNK_WIDTH_LOG2), y + (Y<< H_CHUNK_WIDTH_LOG2), H_MAX_SUN_LIGHT );
    }
}

 void h_World::Blast( short x, short y, short z, short radius )
 {
 	Lock();

 	if( !InBorders( x, y, z ) )
    {
        Unlock();
        return;
    }

	for( short k= z, r=radius; k< z+radius; k++, r-- )
		BlastBlock_r( x, y, k, r );
	for( short k= z-1, r=radius-1; k> z-radius; k--, r-- )
		BlastBlock_r( x, y, k, r );

	for( short i= x - radius; i< x+radius; i++ )
 		for( short j= y - radius; j< y+radius; j++ )
 			for( short k= z - radius; k< z+radius; k++ )
 				RelightBlockRemove( i, j, k );

	UpdateInRadius( x, y, radius );

 	Unlock();
 }

void h_World::BlastBlock_r( short x, short y, short z, short blast_power )
{
	if( blast_power == 0 )
		return;

	h_Chunk* ch= GetChunk( x>>H_CHUNK_WIDTH_LOG2, y>>H_CHUNK_WIDTH_LOG2 );
	unsigned int addr;

	addr= BlockAddr( x&(H_CHUNK_WIDTH-1), y&(H_CHUNK_WIDTH-1), z );
	if( ch->blocks[addr]->Type() != WATER )
	{
		//ch->SetBlockAndTransparency( local_x, local_y, z, NormalBlock(AIR), TRANSPARENCY_AIR );
		ch->blocks[ addr ]= NormalBlock(AIR);
		ch->transparency[ addr ]= TRANSPARENCY_AIR;
	}

	//BlastBlock_r( x, y, z + 1, blast_power-1 );
	//BlastBlock_r( x, y, z - 1, blast_power-1 );
	BlastBlock_r( x, y+1, z, blast_power-1 );
	BlastBlock_r( x, y-1, z, blast_power-1 );
	BlastBlock_r( x+1, y+((x+1)&1), z, blast_power-1 );
	BlastBlock_r( x+1, y-(x&1), z, blast_power-1 );
	BlastBlock_r( x-1, y+((x+1)&1), z, blast_power-1 );
	BlastBlock_r( x-1, y-(x&1), z, blast_power-1 );
}

void h_World::Build( short x, short y, short z, h_BlockType block_type )
{
    if( !InBorders( x, y, z ) )
        return;
    if( !CanBuild( x, y, z ) )
        return;

    short X= x>> H_CHUNK_WIDTH_LOG2, Y = y>> H_CHUNK_WIDTH_LOG2;

    x&= H_CHUNK_WIDTH - 1;
    y&= H_CHUNK_WIDTH - 1;
    if( block_type == WATER )
    {
        h_LiquidBlock* b;
        h_Chunk* ch=GetChunk( X, Y );
        ch-> SetBlockAndTransparency( x, y, z, b= ch->NewWaterBlock(),
                                      TRANSPARENCY_LIQUID );

        b->x= x;
        b->y= y;
        b->z= z;
    }
    else if( block_type == FIRE_STONE )
    {
    	  h_Chunk* ch=GetChunk( X, Y );
    	  h_LightSource* s= ch->NewLightSource( x, y, z, FIRE_STONE );
    	  s->SetLightLevel( H_MAX_FIRE_LIGHT );
    	  ch->SetBlockAndTransparency( x, y, z, s, TRANSPARENCY_SOLID );
    	  AddFireLight_r( x + X* H_CHUNK_WIDTH, y + Y* H_CHUNK_WIDTH, z, H_MAX_FIRE_LIGHT );
	}
    else
        GetChunk( X, Y )->
        SetBlockAndTransparency( x, y, z, NormalBlock( block_type ),
                                 NormalBlock( block_type )->Transparency() );

	short r= 1;
	if( block_type == WATER )
	{
	}
	else
		 r= RelightBlockAdd( X * H_CHUNK_WIDTH + x,Y * H_CHUNK_WIDTH + y, z ) + 1;

	UpdateInRadius( X * H_CHUNK_WIDTH + x,Y * H_CHUNK_WIDTH + y, r );
	UpdateWaterInRadius( X * H_CHUNK_WIDTH + x,Y * H_CHUNK_WIDTH + y, r );

}

void h_World::AddBuildEvent( short x, short y, short z, h_BlockType block_type )
{
	action_queue_mutex.lock();

	h_WorldAction act;
	act.type= ACTION_BUILD;
	act.block_type= block_type;
	act.coord[0]= x;
	act.coord[1]= y;
	act.coord[2]= z;

	action_queue[0].enqueue( act );

	action_queue_mutex.unlock();
}
void h_World::AddDestroyEvent( short x, short y, short z )
{
	action_queue_mutex.lock();

	h_WorldAction act;
	act.type= ACTION_DESTROY;
	act.coord[0]= x;
	act.coord[1]= y;
	act.coord[2]= z;

	action_queue[0].enqueue( act );

	action_queue_mutex.unlock();
}

void h_World::FlushActionQueue()
{
	action_queue_mutex.lock();
	action_queue[0].swap( action_queue[1] );
	action_queue_mutex.unlock();

	while( action_queue[1].size() != 0 )
	{
		h_WorldAction act= action_queue[1].dequeue();
		if( act.type == ACTION_BUILD )
			Build( act.coord[0], act.coord[1], act.coord[2], act.block_type );
		else if( act.type == ACTION_DESTROY )
			Destroy( act.coord[0], act.coord[1], act.coord[2] );
	}
}

void h_World::MoveWorld( h_WorldMoveDirection dir )
{
	int i, j;
	switch ( dir )
	{
		case NORTH:
		for( i= 0; i< chunk_number_x; i++ )
		{
			h_Chunk* deleted_chunk= chunks[ i | ( 0 << H_MAX_CHUNKS_LOG2 ) ];
			SaveChunk( deleted_chunk );
			chunk_loader.FreeChunkData( deleted_chunk->Longitude(), deleted_chunk->Latitude() );
			delete deleted_chunk;
			for( j= 1; j< chunk_number_y; j++ )
			{
				chunks[ i | ( (j-1) << H_MAX_CHUNKS_LOG2 ) ]=
				chunks[ i | ( j << H_MAX_CHUNKS_LOG2 ) ];
			}

			chunks[ i | ( (chunk_number_y-1) << H_MAX_CHUNKS_LOG2 ) ]=
			LoadChunk( i + longitude, chunk_number_y + latitude );
		}
		for( i= 0; i< chunk_number_x; i++ )
			AddLightToBorderChunk( i, chunk_number_y - 1 );
		latitude++;

		break;

		case SOUTH:
		for( i= 0; i< chunk_number_x; i++ )
		{
			h_Chunk* deleted_chunk= chunks[ i | ( (chunk_number_y-1) << H_MAX_CHUNKS_LOG2 ) ];
			SaveChunk( deleted_chunk );
			chunk_loader.FreeChunkData( deleted_chunk->Longitude(), deleted_chunk->Latitude() );
			delete deleted_chunk;
			for( j= chunk_number_y-1; j> 0; j-- )
			{
				chunks[ i | ( j << H_MAX_CHUNKS_LOG2 ) ]=
				chunks[ i | ( (j-1) << H_MAX_CHUNKS_LOG2 ) ];
			}

			chunks[ i | ( 0 << H_MAX_CHUNKS_LOG2 ) ]=
			LoadChunk( i + longitude,  latitude-1 );
		}
		for( i= 0; i< chunk_number_x; i++ )
			AddLightToBorderChunk( i, 0 );
		latitude--;

		break;

		case EAST:
		for( j= 0; j< chunk_number_y; j++ )
		{
			h_Chunk* deleted_chunk= chunks[ 0 | ( j << H_MAX_CHUNKS_LOG2 ) ];
			SaveChunk( deleted_chunk );
			chunk_loader.FreeChunkData( deleted_chunk->Longitude(), deleted_chunk->Latitude() );
			delete deleted_chunk;
			for( i= 1; i< chunk_number_x; i++ )
			{
				chunks[ (i-1) | ( j << H_MAX_CHUNKS_LOG2 ) ]=
				chunks[ i | ( j << H_MAX_CHUNKS_LOG2 ) ];
			}
			chunks[ ( chunk_number_x-1) | ( j << H_MAX_CHUNKS_LOG2 ) ]=
			LoadChunk( longitude+chunk_number_x, latitude + j );
		}
		for( j= 0; j< chunk_number_y; j++ )
			AddLightToBorderChunk( chunk_number_x-1, j );
		longitude++;

		break;

		case WEST:
		for( j= 0; j< chunk_number_y; j++ )
		{
			h_Chunk* deleted_chunk= chunks[ ( chunk_number_x-1) | ( j << H_MAX_CHUNKS_LOG2 ) ];
			SaveChunk( deleted_chunk );
			chunk_loader.FreeChunkData( deleted_chunk->Longitude(), deleted_chunk->Latitude() );
			delete deleted_chunk;
			for( i= chunk_number_x-1; i> 0; i-- )
			{
				chunks[ i | ( j << H_MAX_CHUNKS_LOG2 ) ]=
				chunks[ (i-1) | ( j << H_MAX_CHUNKS_LOG2 ) ];
			}
			chunks[ 0 | ( j << H_MAX_CHUNKS_LOG2 ) ]=
			LoadChunk( longitude-1, latitude + j );
		}
		for( j= 0; j< chunk_number_y; j++ )
			AddLightToBorderChunk( 0, j );
		longitude--;

		break;
	};

	emit FullUpdate();
}


void h_World::SaveChunk( h_Chunk* ch )
{
	/*QByteArray array;
	QDataStream stream( &array, QIODevice::WriteOnly );

	HEXCHUNK_header header;
	//strcpy( header.format_key, "HEXchunk" );
	header.water_block_count= ch->GetWaterList()->Size();
	header.longitude= ch->Longitude();
	header.latitude= ch->Latitude();

	header.Write( stream );

	ch->SaveChunkToFile( stream );

	QByteArray compressed_chunk= qCompress( array );

	char file_name[128];
	sprintf( file_name, "world/ch_%d_%d", ch->Longitude(), ch->Latitude() );
	FILE* f= fopen( file_name, "wb" );
	fwrite( compressed_chunk.constData(), 1, compressed_chunk.size(), f );
	fclose(f);*/

	QByteArray array;
	QDataStream stream( &array, QIODevice::WriteOnly );

	HEXCHUNK_header header;
	//strcpy( header.format_key, "HEXchunk" );
	header.water_block_count= ch->GetWaterList()->Size();
	header.longitude= ch->Longitude();
	header.latitude= ch->Latitude();

	header.Write( stream );
	ch->SaveChunkToFile( stream );

	chunk_loader.GetChunkData( ch->Longitude(), ch->Latitude() )= qCompress( array );
}
h_Chunk* h_World::LoadChunk( int lon, int lat )
{
	/*char file_name[128];
	sprintf( file_name, "world/ch_%d_%d", lon, lat );
	FILE* f= fopen( file_name, "rb" );
	if( f == NULL )
		return  new h_Chunk( this, lon, lat );

	int file_len;
	fseek( f, 0, SEEK_END );
	file_len= ftell( f );
	fseek( f, 0, SEEK_SET );

	unsigned char* file_data= new unsigned char[ file_len ];
	fread( file_data, 1, file_len, f );
	fclose(f);

	QByteArray uncompressed_chunk= qUncompress( file_data, file_len );
	delete[] file_data;
	QDataStream stream( &uncompressed_chunk, QIODevice::ReadOnly );

	HEXCHUNK_header header;
	header.Read( stream );

	return new h_Chunk( this, &header, stream );*/

	QByteArray& ba= chunk_loader.GetChunkData( lon, lat );
	if( ba.size() == 0 )
		return new h_Chunk( this, lon, lat );


	QByteArray uncompressed_chunk= qUncompress( ba );
	QDataStream stream( &uncompressed_chunk, QIODevice::ReadOnly );

	HEXCHUNK_header header;
	header.Read( stream );

	return new h_Chunk( this, &header, stream );
}


h_World::~h_World()
{
    for( unsigned int x= 0; x< chunk_number_x; x++ )
        for( unsigned int y= 0; y< chunk_number_y; y++ )
        {
        	SaveChunk( GetChunk(x,y) );
            delete GetChunk(x,y);
        }
}


void h_World::Save()
{
	 for( unsigned int x= 0; x< chunk_number_x; x++ )
        for( unsigned int y= 0; y< chunk_number_y; y++ )
        	SaveChunk( GetChunk(x,y) );
	chunk_loader.ForceSaveAllChunks();
}
#endif//WORLD_CPP
