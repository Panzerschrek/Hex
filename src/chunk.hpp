#pragma once
#include <vector>

#include "hex.hpp"
#include "fwd.hpp"
#include "block.hpp"
#include "world_loading.hpp"
#include "math_lib/binary_stream.hpp"
#include "math_lib/small_objects_allocator.hpp"

#define BlockAddr( x, y, z ) ( (z) |\
	( (y) << H_CHUNK_HEIGHT_LOG2 ) |\
	( (x) << ( H_CHUNK_HEIGHT_LOG2 + H_CHUNK_WIDTH_LOG2 ) ) )

class h_Chunk
{
	friend class h_World;

public:
	h_Chunk( h_World* world, int longitude, int latitude, const g_WorldGenerator* generator );
	h_Chunk( h_World* world, const HEXCHUNK_header& header, h_BinaryInputStream& stream );

	~h_Chunk();

	//get functions - local coordinates
	unsigned char Transparency( int x, int y, int z ) const;
	const unsigned char* GetTransparencyData() const;

	h_Block* GetBlock( int x, int y, int z );
	const h_Block* GetBlock( int x, int y, int z ) const;
	h_Block* GetBlock( unsigned int addr );
	const h_Block* GetBlock( unsigned int addr ) const;
	const h_Block* const* GetBlocksData() const;

	const std::vector<h_FailingBlock*>& GetFailingBlocks() const;

	const std::vector< h_LiquidBlock* >& GetWaterList() const;
	const std::vector< h_NonstandardFormBlock* >& GetNonstandartFormBlocksList() const;
	const std::vector< h_LightSource* >& GetLightSourceList() const;
	const std::vector< h_Fire* >& GetFireList() const;


	h_World* GetWorld();
	const h_World* GetWorld() const;

	int Longitude() const;
	int Latitude() const;
	bool IsEdgeChunk() const;

	unsigned int GetWaterColumnHeight( int x, int y, int z );
	unsigned char SunLightLevel( int x, int y, int z ) const;
	unsigned char FireLightLevel( int x, int y, int z ) const;

	unsigned char SunLightLevel( unsigned int addr ) const;
	unsigned char FireLightLevel( unsigned int addr ) const;

	// Get sun and fire light levels. out_lights[0]= sun, out_lights[1]= fire
	void GetLightsLevel( int x, int y, int z, unsigned char* out_lights ) const;

	const unsigned char* GetSunLightData () const;
	const unsigned char* GetFireLightData() const;

private:
	void SaveBlock( h_BinaryOuptutStream& stream, const h_Block* block ) const;
	h_Block* LoadBlock( h_BinaryInputStream& stream, unsigned int block_addr );

	void GenChunk( const g_WorldGenerator* generator );

	//chunk save\load
	void GenChunkFromFile( h_BinaryInputStream& stream );
	void SaveChunkToFile( h_BinaryOuptutStream& stream ) const;

	void PlantTrees( const g_WorldGenerator* generator );
	void PlantTree( int x, int y, int z );//local coordinates
	void PlantBigTree( int x, int y, int z );//local coordinates
	void PlantGrass();
	void ActivateGrass();
	unsigned int CalculateWaterBlockCount();
	void GenWaterBlocks();
	void MakeLight();
	void SunRelight();

//water management
	h_LiquidBlock* NewWaterBlock();
	void DeleteWaterBlock( h_LiquidBlock* b );
//lights management
	h_LightSource* NewLightSource( int x, int y, int z, h_BlockType type );
	void DeleteLightSource( h_LightSource* source );
	void DeleteLightSource( int x, int y, int z );

	h_NonstandardFormBlock* NewNonstandardFormBlock(
		unsigned char x, unsigned char y, unsigned char z,
		h_BlockType type, h_Direction direction );

	h_GrassBlock* NewActiveGrassBlock( unsigned char x, unsigned char y, unsigned char z );

	void ProcessFailingBlocks();

	void SetSunLightLevel( int x, int y, int z, unsigned char l );
	void SetFireLightLevel( int x, int y, int z, unsigned char l );

	void SetBlock( int x, int y, int z, h_Block* b );
	void SetBlock( unsigned int addr, h_Block* b );

private:
	h_World* const world_;
	const int longitude_;
	const int latitude_ ;

	bool need_update_light_;

	// TODO - select memory block size for allocatiors

	// water management
	SmallObjectsAllocator< h_LiquidBlock, 256, unsigned char > water_blocks_allocator_;
	std::vector< h_LiquidBlock* > water_block_list_;

	// failing blocks management
	SmallObjectsAllocator< h_FailingBlock, 32, unsigned char > failing_blocks_alocatior_;
	std::vector<h_FailingBlock*> failing_blocks_;

	SmallObjectsAllocator< h_NonstandardFormBlock, 32, unsigned char > nonstandard_form_blocks_allocator_;
	std::vector<h_NonstandardFormBlock*> nonstandard_form_blocks_;

	// Active grass. Grass blocks, which can reproduce, placed here.
	// If grass block has no free space around, it becomes "unactive".
	// Unactive block is unique object, placed in h_World. See h_World::unactive_grass_block_.
	SmallObjectsAllocator< h_GrassBlock, 64, unsigned char > active_grass_blocks_allocator_;
	std::vector<h_GrassBlock*> active_grass_blocks_;

	//light management
	std::vector< h_LightSource* > light_source_list_;

	std::vector< h_Fire* > fire_list_;

	// Large arrays - put back.
	h_Block* blocks_                     [ H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT ];
	h_CombinedTransparency transparency_ [ H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT ];
	unsigned char sun_light_map_         [ H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT ];
	unsigned char fire_light_map_        [ H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT ];

	unsigned char height_map_    [ H_CHUNK_WIDTH * H_CHUNK_WIDTH ];//z coordinates of first nonair block
};

inline unsigned char h_Chunk::Transparency( int x, int y, int z ) const
{
	H_ASSERT( x >= 0 && x < H_CHUNK_WIDTH );
	H_ASSERT( y >= 0 && y < H_CHUNK_WIDTH );
	H_ASSERT( z >= 0 && z < H_CHUNK_HEIGHT );

	return transparency_[ BlockAddr( x, y, z ) ];
}

inline const unsigned char* h_Chunk::GetTransparencyData() const
{
	return transparency_;
}

inline h_Block* h_Chunk::GetBlock( int x, int y, int z )
{
	H_ASSERT( x >= 0 && x < H_CHUNK_WIDTH );
	H_ASSERT( y >= 0 && y < H_CHUNK_WIDTH );
	H_ASSERT( z >= 0 && z < H_CHUNK_HEIGHT );

	return blocks_[ BlockAddr( x, y, z ) ];
}

inline const h_Block* h_Chunk::GetBlock( int x, int y, int z ) const
{
	H_ASSERT( x >= 0 && x < H_CHUNK_WIDTH );
	H_ASSERT( y >= 0 && y < H_CHUNK_WIDTH );
	H_ASSERT( z >= 0 && z < H_CHUNK_HEIGHT );

	return blocks_[ BlockAddr( x, y, z ) ];
}

inline h_Block* h_Chunk::GetBlock( unsigned int addr )
{
	H_ASSERT( addr < H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT );
	return blocks_[addr];
}

inline const h_Block* h_Chunk::GetBlock( unsigned int addr ) const
{
	H_ASSERT( addr < H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT );
	return blocks_[addr];
}

inline const h_Block* const* h_Chunk::GetBlocksData() const
{
	return blocks_;
}

inline const std::vector<h_FailingBlock*>& h_Chunk::GetFailingBlocks() const
{
	return failing_blocks_;
}

inline const std::vector< h_LiquidBlock* >& h_Chunk::GetWaterList() const
{
	return water_block_list_;
}

inline const std::vector< h_NonstandardFormBlock* >& h_Chunk::GetNonstandartFormBlocksList() const
{
	return nonstandard_form_blocks_;
}

inline const std::vector<h_LightSource*>& h_Chunk::GetLightSourceList() const
{
	return light_source_list_;
}

inline const std::vector< h_Fire* >& h_Chunk::GetFireList() const
{
	return fire_list_;
}

inline const h_World* h_Chunk::GetWorld() const
{
	return world_;
}

inline h_World* h_Chunk::GetWorld()
{
	return world_;
}

inline int h_Chunk::Longitude() const
{
	return longitude_;
}

inline int h_Chunk::Latitude () const
{
	return latitude_ ;
}

inline unsigned char h_Chunk::SunLightLevel( int x, int y, int z ) const
{
	H_ASSERT( x >= 0 && x < H_CHUNK_WIDTH );
	H_ASSERT( y >= 0 && y < H_CHUNK_WIDTH );
	H_ASSERT( z >= 0 && z < H_CHUNK_HEIGHT );

	return sun_light_map_[ BlockAddr( x, y, z ) ];
}

inline unsigned char h_Chunk::FireLightLevel( int x, int y, int z ) const
{
	H_ASSERT( x >= 0 && x < H_CHUNK_WIDTH );
	H_ASSERT( y >= 0 && y < H_CHUNK_WIDTH );
	H_ASSERT( z >= 0 && z < H_CHUNK_HEIGHT );

	return fire_light_map_[ BlockAddr( x, y, z ) ];
}

inline unsigned char h_Chunk::SunLightLevel( unsigned int addr ) const
{
	H_ASSERT( addr < H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT );

	return sun_light_map_[ addr ];
}

inline unsigned char h_Chunk::FireLightLevel( unsigned int addr ) const
{
	H_ASSERT( addr < H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT );

	return fire_light_map_[ addr ];
}

inline void h_Chunk::GetLightsLevel( int x, int y, int z, unsigned char* out_lights ) const
{
	H_ASSERT( x >= 0 && x < H_CHUNK_WIDTH );
	H_ASSERT( y >= 0 && y < H_CHUNK_WIDTH );
	H_ASSERT( z >= 0 && z < H_CHUNK_HEIGHT );

	int addr= BlockAddr( x, y, z );
	out_lights[0]= sun_light_map_ [addr];
	out_lights[1]= fire_light_map_[addr];
}

inline const unsigned char* h_Chunk::GetSunLightData() const
{
	return sun_light_map_;
}

inline const unsigned char* h_Chunk::GetFireLightData() const
{
	return fire_light_map_;
}

inline void h_Chunk::SetSunLightLevel( int x, int y, int z, unsigned char l )
{
	H_ASSERT( x >= 0 && x < H_CHUNK_WIDTH );
	H_ASSERT( y >= 0 && y < H_CHUNK_WIDTH );
	H_ASSERT( z >= 0 && z < H_CHUNK_HEIGHT );

	sun_light_map_[ BlockAddr( x, y, z ) ]= l;
}

inline void h_Chunk::SetFireLightLevel( int x, int y, int z, unsigned char l )
{
	H_ASSERT( x >= 0 && x < H_CHUNK_WIDTH );
	H_ASSERT( y >= 0 && y < H_CHUNK_WIDTH );
	H_ASSERT( z >= 0 && z < H_CHUNK_HEIGHT );

	fire_light_map_[ BlockAddr( x, y, z ) ]= l;
}

inline void h_Chunk::SetBlock( int x, int y, int z, h_Block* b )
{
	H_ASSERT( x >= 0 && x < H_CHUNK_WIDTH );
	H_ASSERT( y >= 0 && y < H_CHUNK_WIDTH );
	H_ASSERT( z >= 0 && z < H_CHUNK_HEIGHT );

	const int addr= BlockAddr( x, y, z );

	transparency_[addr]= b->CombinedTransparency();
	blocks_[addr]= b;
}

inline void h_Chunk::SetBlock( unsigned int addr, h_Block* b )
{
	H_ASSERT( addr < H_CHUNK_WIDTH * H_CHUNK_WIDTH * H_CHUNK_HEIGHT );

	transparency_[addr]= b->CombinedTransparency();
	blocks_[addr]= b;
}
