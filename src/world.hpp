#ifndef WORLD_HPP
#define WORLD_HPP

#include <QMutex>

#include "hex.hpp"
#include "block.hpp"
#include "chunk.hpp"
#include "thread.hpp"
#include "chunk_phys_mesh.hpp"


class h_Player;

class h_World : public QObject
{
    Q_OBJECT

    friend class h_Chunk;

public:
    h_Chunk* GetChunk( short X, short Y );//relative chunk coordinates
    unsigned int ChunkNumberX();
    unsigned int ChunkNumberY();
    void Build( short x, short y, short z, h_BlockType block_type );//coordinates - relative
    void Destroy( short x, short y, short z );

	void SetPlayer( h_Player* p );
    short Longitude();
    short Latitude();

    h_World();
    ~h_World();

	void Lock();
	void Unlock();


	//returns light level of forward-forwardright upper vertex of prism X16. coordinates - relative
	unsigned char GetForwardVertexSunLight( short x, short y, short z );
	//returns light level of back-backleft upper vertex of prism X16. coordinates - relative
	unsigned char GetBackVertexSunLight( short x, short y, short z );
	//returns light level of forward-forwardright upper vertex of prism X16. coordinates - relative
	unsigned char GetForwardVertexFireLight( short x, short y, short z );
	//returns light level of back-backleft upper vertex of prism X16. coordinates - relative
	unsigned char GetBackVertexFireLight( short x, short y, short z );
	//returns nominal value of lightmaps
	unsigned char SunLightLevel( short x, short y, short z );
	unsigned char FireLightLevel( short x, short y, short z );


    signals:
    void ChunkUpdated( unsigned short, unsigned short );
    void ChunkWaterUpdated( unsigned short, unsigned short );
private:
	void UpdateInRadius( short x, short y, short r );//update chunks in square [x-r;x+r] [y-r;x+r]

	//function uses local coordinates of loaded zone
	void BuildPhysMesh( h_ChunkPhysMesh* phys_mesh, short x_min, short x_max, short y_min, short y_max, short z_min, short z_max );

	//lighting. relative coordinates
	void LightWorld();

	void AddSunLight_r( short x, short y, short z, unsigned char l );
	void AddFireLight_r( short x, short y, short z, unsigned char l );
	//clamp coordinates to [ 0; H_CHUNK_WIDTH * chunk_number - 1 ) for x and y
	//and to [ 0; H_CHUNK_HEIGHT - 1 ] for z
	short ClampX( short x );
	short ClampY( short y );
	short ClampZ( short z );
	//safe versions of lighting methods.
	void AddSunLightSafe_r( short x, short y, short z, unsigned char l );
	void AddFireLightSafe_r( short x, short y, short z, unsigned char l );
	void RelightBlockRemove( short x, short y, short z );
	//return update radius
	short RelightBlockAdd( short x, short y, short z );

	void SetSunLightLevel( short x, short y, short z, unsigned char l );
	void SetFireLightLevel( short x, short y, short z, unsigned char l );


	bool InBorders( short x, short y, short z );
	bool CanBuild( short x, short y, short z );

	void WaterPhysTick();
	bool WaterFlow( h_LiquidBlock* from, short to_x, short to_y, short to_z ); //returns true if chunk was midifed
	bool WaterFlowDown( h_LiquidBlock* from, short to_x, short to_y, short to_z );
	bool WaterFlowUp( h_LiquidBlock* from, short to_x, short to_y, short to_z );


    h_Block* NormalBlock( h_BlockType block_type );
   // h_Block* WaterBlock( unsigned char water_level= MAX_WATER_LEVEL );
    void InitNormalBlocks();

    unsigned int chunk_number_x, chunk_number_y;
    unsigned int chunk_matrix_size_x, chunk_matrix_size_x_log2;
    h_Chunk** chunks;//array of pointers
    int longitude, latitude;//loaded zone beginning longitude and latitude


	h_Player* player;
	short player_coord[3];//global coordinate of player hexagon

	h_ChunkPhysMesh player_phys_mesh;

    h_Block normal_blocks[ NUM_BLOCK_TYPES ];

	unsigned int phys_tick_count;
	h_Thread< h_World > phys_thread;
	void PhysTick();
	QMutex world_mutex;

};


inline  unsigned int h_World::ChunkNumberX()
{
    return chunk_number_x;
}

inline  unsigned int h_World::ChunkNumberY()
{
    return chunk_number_y;
}

inline h_Chunk* h_World::GetChunk( short X, short Y )
{
    return chunks[ X | ( Y << chunk_matrix_size_x_log2 ) ];
}

inline h_Block* h_World::NormalBlock( h_BlockType block_type )
{
    return &normal_blocks[ block_type ];
    if( block_type  > 6 )
        printf( "error" );
}

inline void h_World::SetPlayer( h_Player* p )
{
	player= p;
}
/*inline h_Block* h_World::WaterBlock( unsigned char water_level )
{
    return &water_blocks[ water_level ];
}*/

inline short h_World::Longitude()
{
    return longitude;
}

inline short h_World::Latitude()
{
    return latitude;
}


inline short h_World::ClampX( short x )
{
	if( x < 0 )
		return 0;
	short max_x= chunk_number_x * H_CHUNK_WIDTH - 1;
	if ( x > max_x )
		return max_x;
	return x;
}

inline short h_World::ClampY( short y )
{
	if( y < 0 )
		return 0;
	short max_y= chunk_number_y * H_CHUNK_WIDTH - 1;
	if ( y > max_y )
		return max_y;
	return y;
}
inline short h_World::ClampZ( short z )
{
	if( z < 0 )
		return 0;
	if( z > H_CHUNK_HEIGHT - 1 )
		return H_CHUNK_HEIGHT - 1;
	return z;
}

#endif//WORLD_HPP
