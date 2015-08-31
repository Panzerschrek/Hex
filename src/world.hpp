#pragma once
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include <QSettings>

#include "hex.hpp"
#include "fwd.hpp"
#include "block.hpp"
#include "chunk.hpp"
#include "chunk_phys_mesh.hpp"
#include "math_lib/rand.h"
#include "world_action.hpp"
#include "chunk_loader.hpp"

class h_World : public QObject
{
	friend class h_Chunk;

public:
	h_World();
	~h_World();

	h_Chunk* GetChunk( short X, short Y );//relative chunk coordinates
	const h_Chunk* GetChunk( short X, short Y ) const;//relative chunk coordinates

	//really, returns longitude/2 and latitude/2
	int ChunkCoordToQuadchunkX( int longitude ) const;
	int ChunkCoordToQuadchunkY( int latitude ) const;

	int ChunkNumberX() const;
	int ChunkNumberY() const;

	short Longitude() const;
	short Latitude() const;

	void AddBuildEvent( short x, short y, short z, h_BlockType block_type );//coordinates - relative
	void AddDestroyEvent( short x, short y, short z );

	//replace all blocks in radius in this layer( z=const )
	//ACHTUNG! This function unfinished. It ignores destruction of light sources. also, danger of stack owerflow. time ~ 6^radius
	void Blast( short x, short y, short z, short radius );

	void SetPlayer( h_Player* p );
	void SetRenderer( r_IWorldRenderer* r );
	void StartUpdates(); // start main phys loop of world

	void Save();//save world data to disk

	unsigned char SunLightLevel( short x, short y, short z ) const;
	unsigned char FireLightLevel( short x, short y, short z ) const;
	//returns light level of forward-forwardright upper vertex of prism X16. coordinates - relative
	void GetForwardVertexLight( short x, short y, short z, unsigned char* out_light ) const;
	//returns light level of back-backleft upper vertex of prism X16. coordinates - relative
	void GetBackVertexLight( short x, short y, short z, unsigned char* out_light ) const;
	//returns nominal value of lightmaps

private:
	void Build( short x, short y, short z, h_BlockType block_type );//coordinates - relative
	void Destroy( short x, short y, short z );
	void FlushActionQueue();

	void UpdateInRadius( short x, short y, short r );//update chunks in square [x-r;x+r] [y-r;x+r]
	void UpdateWaterInRadius( short x, short y, short r );//update chunks water in square [x-r;x+r] [y-r;x+r]

	void MoveWorld( h_WorldMoveDirection dir );
	void SaveChunk( h_Chunk* ch );
	h_Chunk* LoadChunk( int longitude, int lattude );

	//coordinates of chunks in chunk matrix
	void AddLightToBorderChunk( unsigned int X, unsigned int Y );

	//function uses local coordinates of loaded zone
	void BuildPhysMesh( h_ChunkPhysMesh* phys_mesh, short x_min, short x_max, short y_min, short y_max, short z_min, short z_max );

	//clamp coordinates to [ 0; H_CHUNK_WIDTH * chunk_number - 1 ) for x and y
	//and to [ 0; H_CHUNK_HEIGHT - 1 ] for z
	short ClampX( short x ) const;
	short ClampY( short y ) const;
	short ClampZ( short z ) const;
	//safe versions of lighting methods.

	//lighting. relative coordinates
	void SetSunLightLevel( short x, short y, short z, unsigned char l );
	void SetFireLightLevel( short x, short y, short z, unsigned char l );

	void LightWorld();
	//return update radius
	short RelightBlockAdd( short x, short y, short z );
	void RelightBlockRemove( short x, short y, short z );

	void AddSunLight_r( short x, short y, short z, unsigned char l );
	void AddFireLight_r( short x, short y, short z, unsigned char l );
	void AddSunLightSafe_r( short x, short y, short z, unsigned char l );
	void AddFireLightSafe_r( short x, short y, short z, unsigned char l );

	//add light from light sources in cube
	void ShineFireLight( short x_min, short y_min, short z_min, short x_max, short y_max, short z_max );

	void BlastBlock_r( short x, short y, short z, short blast_power );
	bool InBorders( short x, short y, short z ) const;
	bool CanBuild( short x, short y, short z ) const;

	void PhysTick();

	void RelightWaterModifedChunksLight();//relight chunks, where water was modifed in last ticks
	void WaterPhysTick();
	bool WaterFlow( h_LiquidBlock* from, short to_x, short to_y, short to_z ); //returns true if chunk was midifed
	bool WaterFlowDown( h_LiquidBlock* from, short to_x, short to_y, short to_z );
	bool WaterFlowUp( h_LiquidBlock* from, short to_x, short to_y, short to_z );

	h_Block* NormalBlock( h_BlockType block_type );
	// h_Block* WaterBlock( unsigned char water_level= MAX_WATER_LEVEL );
	void InitNormalBlocks();

private:
	h_ChunkLoader chunk_loader_;

	unsigned int chunk_number_x_, chunk_number_y_;
	unsigned int chunk_matrix_size_x_, chunk_matrix_size_x_log2_;
	h_Chunk* chunks_[ H_MAX_CHUNKS * H_MAX_CHUNKS ];//matrix of pointers
	int longitude_, latitude_;//loaded zone beginning longitude and latitude

	m_Rand phys_processes_rand_;

	r_IWorldRenderer* renderer_;
	h_Player* player_;
	short player_coord_[3];//global coordinate of player hexagon

	h_ChunkPhysMesh player_phys_mesh_;

	h_Block normal_blocks_[ NUM_BLOCK_TYPES ];

	unsigned int phys_tick_count_;
	std::unique_ptr< std::thread > phys_thread_;

	//queue 0 - for enqueue, queue 1 - for dequeue
	std::queue< h_WorldAction > action_queue_[2];
	std::mutex action_queue_mutex_;

	QSettings settings_;

};

inline int h_World::ChunkNumberX() const
{
	return chunk_number_x_;
}

inline int h_World::ChunkNumberY() const
{
	return chunk_number_y_;
}

inline short h_World::Longitude() const
{
	return longitude_;
}

inline short h_World::Latitude() const
{
	return latitude_;
}

inline h_Chunk* h_World::GetChunk( short X, short Y )
{
	return chunks_[ X | ( Y << H_MAX_CHUNKS_LOG2 ) ];
}
inline const h_Chunk* h_World::GetChunk( short X, short Y ) const
{
	return chunks_[ X | ( Y << H_MAX_CHUNKS_LOG2 ) ];
}

inline h_Block* h_World::NormalBlock( h_BlockType block_type )
{
	return &normal_blocks_[ (int)block_type ];
}

inline void h_World::SetPlayer( h_Player* p )
{
	player_= p;
}

inline void h_World::SetRenderer( r_IWorldRenderer* r )
{
	renderer_= r;
}

/*inline h_Block* h_World::WaterBlock( unsigned char water_level )
{
	return &water_blocks[ water_level ];
}*/

inline short h_World::ClampX( short x ) const
{
	if( x < 0 )
		return 0;
	short max_x= chunk_number_x_ * H_CHUNK_WIDTH - 1;
	if ( x > max_x )
		return max_x;
	return x;
}

inline short h_World::ClampY( short y ) const
{
	if( y < 0 )
		return 0;
	short max_y= chunk_number_y_ * H_CHUNK_WIDTH - 1;
	if ( y > max_y )
		return max_y;
	return y;
}
inline short h_World::ClampZ( short z ) const
{
	if( z < 0 )
		return 0;
	if( z > H_CHUNK_HEIGHT - 1 )
		return H_CHUNK_HEIGHT - 1;
	return z;
}

inline int h_World::ChunkCoordToQuadchunkX( int longitude ) const
{
	return longitude_>>1;
}

inline int h_World::ChunkCoordToQuadchunkY( int latitude ) const
{
	return latitude_>>1;
}
