#pragma once
#include "hex.hpp"
#include "fwd.hpp"
#include "block.hpp"
#include "math_lib/collection.hpp"
#include "world_loading.hpp"

#define BlockAddr( x, y, z ) ( (z) |\
	( (y) << H_CHUNK_HEIGHT_LOG2 ) |\
	( (x) << ( H_CHUNK_HEIGHT_LOG2 + H_CHUNK_WIDTH_LOG2 ) ) )


class h_Chunk
{

	friend class h_World;

public:

	h_Chunk( h_World* world, int longitude, int latitude );
	h_Chunk( h_World* world, const HEXCHUNK_header* header, QDataStream& stream  );

	~h_Chunk();

	//get functions - local coordinates
	unsigned char Transparency( short x, short y, short z ) const;
	const unsigned char* GetTransparencyData() const;
	h_Block* GetBlock( short x, short y, short z );
	const h_Block* GetBlock( short x, short y, short z ) const;
	const m_Collection< h_LiquidBlock* >* GetWaterList() const;
	const m_Collection< h_LightSource* >* GetLightSourceList() const;
	h_World* GetWorld();
	const h_World* GetWorld() const;


	short Longitude() const;
	short Latitude() const;
	bool IsEdgeChunk() const;

	unsigned int GetWaterColumnHeight( short x, short y, short z );
	unsigned char SunLightLevel( short x, short y, short z ) const;
	unsigned char FireLightLevel( short x, short y, short z ) const;

private:
	void GenChunk();

	//chunk save\load
	void GenChunkFromFile( const HEXCHUNK_header* header, QDataStream& stream );
	void SaveChunkToFile( QDataStream& stream );

	void PlantTrees();
	void PlaneBigTree( short x, short y, short z );//local coordinates
	void PlantGrass();
	unsigned int CalculateWaterBlockCount();
	void GenWaterBlocks();
	void MakeLight();
	void SunRelight();

	void PrepareWaterBlocksCache( int needed_block_count );


	void ReCalculateHeightmap();

//water management
	h_LiquidBlock* NewWaterBlock();
	void DeleteWaterBlock( h_LiquidBlock* b );
//lights management
	h_LightSource* NewLightSource( short x, short y, short z, h_BlockType type );
	void DeleteLightSource( short x, short y, short z );

	void SetSunLightLevel( short x, short y, short z, unsigned char l );
	void SetFireLightLevel( short x, short y, short z, unsigned char l );

	void SetBlock( short x, short y, short z, h_Block* b );
	void SetBlockAndTransparency( short x, short y, short z, h_Block* b, h_TransparencyType t );
	void SetTransparency( short x, short y, short z, h_TransparencyType t );

private:
	h_World* const world_;
	const int longitude_;
	const int latitude_ ;

	bool need_update_light_;

	//water management
	struct
	{
		h_LiquidBlock* initial_water_blocks;
		unsigned int initial_water_block_buffer_size;
		unsigned int free_blocks_position;//offset from 'initial_water_blocks', where is free space for new water blocks
		m_Collection< h_LiquidBlock* > water_block_list;
	} water_blocks_data;

	//light management
	m_Collection< h_LightSource* > light_source_list_;

	// Large arrays - put back.
	h_Block* blocks_             [ H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT ];
	unsigned char transparency_  [ H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT ];
	unsigned char sun_light_map_ [ H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT ];
	unsigned char fire_light_map_[ H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT ];

	unsigned char height_map_    [ H_CHUNK_WIDTH * H_CHUNK_WIDTH ];//z coordinates of first nonair block
};


inline const m_Collection< h_LiquidBlock* >* h_Chunk::GetWaterList() const
{
	return & water_blocks_data.water_block_list;
}

inline const m_Collection< h_LightSource* >* h_Chunk::GetLightSourceList() const
{
	return &light_source_list_;
}

inline unsigned char h_Chunk::Transparency( short x, short y, short z ) const
{
	return transparency_[ BlockAddr( x, y, z ) ];
}

inline h_Block* h_Chunk::GetBlock( short x, short y, short z )
{
	return blocks_[ BlockAddr( x, y, z ) ];
}

inline const h_Block* h_Chunk::GetBlock( short x, short y, short z ) const
{
	return blocks_[ BlockAddr( x, y, z ) ];
}


inline void h_Chunk::SetBlock( short x, short y, short z, h_Block* b )
{
	blocks_[ BlockAddr( x, y, z ) ]= b;
}

inline void h_Chunk::SetTransparency( short x, short y, short z, h_TransparencyType t )
{
	transparency_[ BlockAddr( x, y, z ) ]= t;
}

inline void h_Chunk::SetBlockAndTransparency( short x, short y, short z, h_Block* b, h_TransparencyType t )
{
	short addr= BlockAddr( x, y, z );

	transparency_[addr]= t;
	blocks_[addr]= b;
}

inline unsigned char h_Chunk::SunLightLevel( short x, short y, short z ) const
{
	return sun_light_map_[ BlockAddr( x, y, z ) ];
}

inline unsigned char h_Chunk::FireLightLevel( short x, short y, short z ) const
{
	return fire_light_map_[ BlockAddr( x, y, z ) ];
}

inline void h_Chunk::SetSunLightLevel( short x, short y, short z, unsigned char l )
{
	sun_light_map_[ BlockAddr( x, y, z ) ]= l;
}
inline void h_Chunk::SetFireLightLevel( short x, short y, short z, unsigned char l )
{
	fire_light_map_[ BlockAddr( x, y, z ) ]= l;
}

inline const unsigned char* h_Chunk::GetTransparencyData() const
{
	return transparency_;
}

inline short h_Chunk::Longitude() const
{
	return longitude_;
}
inline short h_Chunk::Latitude () const
{
	return latitude_ ;
}

inline const h_World* h_Chunk::GetWorld() const
{
	return world_;
}

inline h_World* h_Chunk::GetWorld()
{
	return world_;
}
